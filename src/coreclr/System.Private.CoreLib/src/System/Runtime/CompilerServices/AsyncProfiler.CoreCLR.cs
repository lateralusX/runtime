// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System.Buffers;
using System.Diagnostics;
using System.Diagnostics.Tracing;
using Serializer = System.Runtime.CompilerServices.AsyncProfiler.EventBuffer.Serializer;

namespace System.Runtime.CompilerServices
{
    internal static partial class AsyncProfiler
    {
        internal static partial class CreateAsyncContext
        {
            public static void Create(ulong id, Continuation nextContinuation)
            {
                Info info = default;
                AsyncThreadContext context = AsyncThreadContext.Acquire(ref info);

                SyncPoint.Check(context);

                EventKeywords eventKeywords = context.ActiveEventKeywords;
                long currentTimestamp = Stopwatch.GetTimestamp();

                if (IsEnabled.CreateAsyncContextEvent(eventKeywords))
                {
                    EmitEvent(context, currentTimestamp, id);
                }

                if (IsEnabled.CreateAsyncCallstackEvent(eventKeywords))
                {
                    AsyncCallstack.EmitEvent(context, currentTimestamp, AsyncEventID.CreateAsyncCallstack, id, nextContinuation);
                }

                AsyncThreadContext.Release(context);
            }
        }

        internal static partial class ResumeAsyncContext
        {
            public static void Resume(ref AsyncDispatcherInfo info)
            {
                AsyncThreadContext context = AsyncThreadContext.Acquire(ref info.AsyncProfilerInfo);

                if (SyncPoint.Check(context))
                {
                    AsyncThreadContext.Release(context);
                    return;
                }

                EventKeywords activeEventKeywords = context.ActiveEventKeywords;
                long currentTimestamp = Stopwatch.GetTimestamp();
                ulong id = GetTaskId(ref info);

                if (IsEnabled.ResumeAsyncContextEvent(activeEventKeywords))
                {
                    PerfStats.RecordStart(out long perfStart);
                    EmitEvent(context, currentTimestamp, id);
                    PerfStats.RecordEnd(context, AsyncEventID.ResumeAsyncContext, perfStart);
                }

                if (IsEnabled.ResumeAsyncCallstackEvent(activeEventKeywords))
                {
                    PerfStats.RecordStart(out long perfStart);
                    AsyncCallstack.EmitEvent(context, currentTimestamp, id, info.NextContinuation);
                    PerfStats.RecordEnd(context, AsyncEventID.ResumeAsyncCallstack, perfStart);
                }

                AsyncThreadContext.Release(context);
            }

            public static void Resume(ref AsyncDispatcherInfo info, AsyncThreadContext context, EventKeywords activeEventKeywords)
            {
                if (SyncPoint.Check(context))
                {
                    return;
                }

                long currentTimestamp = Stopwatch.GetTimestamp();
                ulong id = GetTaskId(ref info);

                if (IsEnabled.ResumeAsyncContextEvent(activeEventKeywords))
                {
                    PerfStats.RecordStart(out long perfStart);
                    EmitEvent(context, currentTimestamp, id);
                    PerfStats.RecordEnd(context, AsyncEventID.ResumeAsyncContext, perfStart);
                }

                if (IsEnabled.ResumeAsyncCallstackEvent(activeEventKeywords))
                {
                    AsyncCallstack.EmitEvent(context, currentTimestamp, id, info.NextContinuation);
                }
            }
        }

        internal static partial class SuspendAsyncContext
        {
            public static void Suspend(ref AsyncDispatcherInfo info, Continuation nextContinuation)
            {
                AsyncThreadContext context = AsyncThreadContext.Acquire(ref info.AsyncProfilerInfo);

                SyncPoint.Check(context);

                EventKeywords activeEventKeywords = context.ActiveEventKeywords;
                long currentTimestamp = Stopwatch.GetTimestamp();

                if (IsEnabled.SuspendAsyncContextEvent(activeEventKeywords))
                {
                    PerfStats.RecordStart(out long perfStart);
                    EmitEvent(context, currentTimestamp);
                    PerfStats.RecordEnd(context, AsyncEventID.SuspendAsyncContext, perfStart);
                }

                if (IsEnabled.SuspendAsyncCallstackEvent(activeEventKeywords))
                {
                    AsyncCallstack.EmitEvent(context, currentTimestamp, AsyncEventID.SuspendAsyncCallstack, GetTaskId(ref info), nextContinuation);
                }

                AsyncThreadContext.Release(context);
            }
        }

        /// <summary>
        /// Provides a table of 32 functionally identical continuation wrapper methods, each with
        /// a unique native IP address. When resuming an async continuation, the profiler dispatches
        /// through the wrapper at index (ContinuationIndex &amp; COUNT_MASK), then increments the index.
        ///
        /// This creates a rotating pattern of unique return addresses on the native callstack. An OS
        /// CPU profiler (e.g., ETW, perf) captures these native IPs in its stack samples. The async
        /// profiler emits the wrapper name template and count in the metadata event, so a post-processing
        /// tool can format the template with each index (0..COUNT-1) to produce method names, resolve
        /// them via symbol data (rundown events or PDB), and correlate native stack IPs with the
        /// async resume callstack events emitted at the same logical point. This bridges the gap
        /// between synchronous native stack samples and the asynchronous continuation chain.
        ///
        /// Every COUNT (32) continuations, a ResetAsyncContinuationWrapperIndex event is emitted
        /// so the tool knows the index has wrapped around and can correctly map subsequent samples.
        ///
        /// Each wrapper is marked [NoInlining] to guarantee a distinct native IP, and
        /// [AggressiveOptimization] to ensure stable JIT output (skip tiered compilation).
        /// </summary>
        [StackTraceHidden]
        internal static partial class ContinuationWrapper
        {
            /// <summary>
            /// Name template for the continuation wrapper methods. External tools format this template
            /// with the wrapper index (0..COUNT-1) to produce method names for identifying wrapper frames in stacks.
            /// Must match the actual method names below (e.g., Continuation_Wrapper_0, Continuation_Wrapper_1, ...).
            /// </summary>
            public const string NameTemplate = "Continuation_Wrapper_{0}";

            /// <summary>
            /// Pre-encoded UTF8 bytes of <see cref="NameTemplate"/> for zero-allocation metadata emission.
            /// </summary>
            public static ReadOnlySpan<byte> NameTemplateUtf8 => "Continuation_Wrapper_{0}"u8;

            public const byte COUNT = 32;
            public const byte COUNT_MASK = COUNT - 1;

            public static void InitInfo(ref Info info)
            {
                info.ContinuationTable = ref Unsafe.As<ContinuationWrapperTable, nint>(ref s_continuationWrappers);
                info.ContinuationIndex = 0;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static Continuation? Dispatch(ref AsyncDispatcherInfo info, Continuation curContinuation, ref byte resultLoc)
            {
                nint dispatcher = Unsafe.Add(ref info.AsyncProfilerInfo.ContinuationTable, info.AsyncProfilerInfo.ContinuationIndex & COUNT_MASK);
                unsafe
                {
                    return ((delegate*<Continuation, ref byte, Continuation?>)(dispatcher))(curContinuation, ref resultLoc);
                }
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_0(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_1(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_2(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_3(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_4(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_5(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_6(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_7(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_8(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_9(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_10(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_11(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_12(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_13(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_14(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_15(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_16(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_17(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_18(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_19(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_20(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_21(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_22(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_23(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_24(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_25(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_26(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_27(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_28(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_29(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_30(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.AggressiveOptimization)]
            private static unsafe Continuation? Continuation_Wrapper_31(Continuation continuation, ref byte resultLoc)
            {
                return continuation.ResumeInfo->Resume(continuation, ref resultLoc);
            }

            private static unsafe ContinuationWrapperTable InitContinuationWrappers()
            {
                ContinuationWrapperTable wrappers = default;
                wrappers[0] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_0;
                wrappers[1] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_1;
                wrappers[2] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_2;
                wrappers[3] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_3;
                wrappers[4] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_4;
                wrappers[5] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_5;
                wrappers[6] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_6;
                wrappers[7] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_7;
                wrappers[8] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_8;
                wrappers[9] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_9;
                wrappers[10] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_10;
                wrappers[11] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_11;
                wrappers[12] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_12;
                wrappers[13] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_13;
                wrappers[14] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_14;
                wrappers[15] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_15;
                wrappers[16] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_16;
                wrappers[17] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_17;
                wrappers[18] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_18;
                wrappers[19] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_19;
                wrappers[20] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_20;
                wrappers[21] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_21;
                wrappers[22] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_22;
                wrappers[23] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_23;
                wrappers[24] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_24;
                wrappers[25] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_25;
                wrappers[26] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_26;
                wrappers[27] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_27;
                wrappers[28] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_28;
                wrappers[29] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_29;
                wrappers[30] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_30;
                wrappers[31] = (nint)(delegate*<Continuation, ref byte, Continuation?>)&Continuation_Wrapper_31;
                return wrappers;
            }

            [InlineArray(COUNT)]
            private struct ContinuationWrapperTable
            {
                private nint _element;
            }

            private static ContinuationWrapperTable s_continuationWrappers = InitContinuationWrappers();
        }

        private static partial class SyncPoint
        {
            private static unsafe void ResumeAsyncCallstacks(AsyncThreadContext context)
            {
                //Write recursively all the resume async callstack events.
                AsyncDispatcherInfo* info = AsyncDispatcherInfo.t_current;
                if (info != null)
                {
                    ResumeRuntimeAsyncCallstacks(info, context);
                }

            }

            private static unsafe void ResumeRuntimeAsyncCallstacks(AsyncDispatcherInfo* info, AsyncThreadContext context)
            {
                if (info != null)
                {
                    ResumeRuntimeAsyncCallstacks(info->Next, context);
                    ResumeAsyncContext.Resume(ref *info, context, Config.ActiveEventKeywords);
                }
            }
        }

        private static partial class AsyncCallstack
        {
            private const int MaxAsyncMethodFrameSize = Serializer.MaxCompressedUInt64Size + Serializer.MaxCompressedUInt32Size;

            public ref struct CaptureRuntimeAsyncCallstackState
            {
                public Continuation? Continuation;
                public ulong LastNativeIP;
                public byte Count;
            }

            public static bool CaptureRuntimeAsyncCallstack(byte[] buffer, ref int index, ref CaptureRuntimeAsyncCallstackState state)
            {
                if (index > buffer.Length || state.Continuation == null)
                {
                    return false;
                }

                byte maxAsyncCallstackFrames = (byte)Math.Min(byte.MaxValue, (buffer.Length - index) / MaxAsyncMethodFrameSize);
                if (maxAsyncCallstackFrames == 0)
                {
                    return false;
                }

                ulong currentNativeIP = 0;
                ulong previousNativeIP = state.LastNativeIP;

                unsafe
                {
                    currentNativeIP = (ulong)state.Continuation.ResumeInfo->DiagnosticIP;
                }

                Span<byte> callstackSpan = buffer.AsSpan(index);
                int callstackSpanIndex = 0;

                // First frame (Count == 0) is written as absolute; subsequent frames
                // (including the first frame of a continuation call after overflow)
                // are written as deltas from the previous frame.
                if (state.Count == 0)
                {
                    callstackSpanIndex += Serializer.WriteCompressedUInt64(callstackSpan.Slice(callstackSpanIndex, Serializer.MaxCompressedUInt64Size), currentNativeIP);
                }
                else
                {
                    callstackSpanIndex += Serializer.WriteCompressedInt64(callstackSpan.Slice(callstackSpanIndex, Serializer.MaxCompressedInt64Size), (long)(currentNativeIP - previousNativeIP));
                }

                callstackSpanIndex += Serializer.WriteCompressedInt32(callstackSpan.Slice(callstackSpanIndex, Serializer.MaxCompressedInt32Size), state.Continuation.State);
                state.Count++;

                state.Continuation = state.Continuation.Next;
                while (state.Count < maxAsyncCallstackFrames && state.Continuation != null)
                {
                    previousNativeIP = currentNativeIP;

                    unsafe
                    {
                        currentNativeIP = (ulong)state.Continuation.ResumeInfo->DiagnosticIP;
                    }

                    callstackSpanIndex += Serializer.WriteCompressedInt64(callstackSpan.Slice(callstackSpanIndex, Serializer.MaxCompressedInt64Size), (long)(currentNativeIP - previousNativeIP));
                    callstackSpanIndex += Serializer.WriteCompressedInt32(callstackSpan.Slice(callstackSpanIndex, Serializer.MaxCompressedInt32Size), state.Continuation.State);

                    state.Count++;
                    state.Continuation = state.Continuation.Next;
                }

                state.LastNativeIP = currentNativeIP;
                index += callstackSpanIndex;

                return state.Continuation == null || state.Count == byte.MaxValue;
            }

            public static void EmitEvent(AsyncThreadContext context, long currentTimestamp, ulong id, Continuation? asyncCallstack)
            {
                EmitEvent(context, currentTimestamp, AsyncEventID.ResumeAsyncCallstack, id, AsyncCallstackType.Runtime, asyncCallstack);
            }

            public static void EmitEvent(AsyncThreadContext context, long currentTimestamp, AsyncEventID eventID, ulong id, Continuation? asyncCallstack)
            {
                EmitEvent(context, currentTimestamp, eventID, id, AsyncCallstackType.Runtime, asyncCallstack);
            }

            public static void EmitEvent(AsyncThreadContext context, long currentTimestamp, AsyncEventID eventID, ulong id, AsyncCallstackType type, Continuation? asyncCallstack)
            {
                if (asyncCallstack != null)
                {
                    ref EventBuffer eventBuffer = ref context.EventBuffer;

                    CaptureRuntimeAsyncCallstackState state = default;
                    state.Continuation = asyncCallstack;

                    // Static callstack payload: type (1) + callstackId (1) + frameCount (1) + id (max 10 bytes compressed).
                    const int MaxStaticEventPayloadSize = sizeof(byte) + sizeof(byte) + sizeof(byte) + Serializer.MaxCompressedUInt64Size;
                    int index = Serializer.BeginAsyncEvent(context, ref eventBuffer, currentTimestamp, eventID, MaxStaticEventPayloadSize);
                    if (index != -1)
                    {
                        EmitAsyncCallstack(context, ref eventBuffer, index, currentTimestamp, eventID, id, type, ref state);
                    }
                }
            }

            private static void EmitAsyncCallstack(AsyncThreadContext context, ref EventBuffer eventBuffer, int startIndex, long currentTimestamp, AsyncEventID eventID, ulong id, AsyncCallstackType type, ref CaptureRuntimeAsyncCallstackState state)
            {
                byte[] buffer = eventBuffer.Data;
                int frameCountOffset = CallstackHeader(buffer, ref startIndex, id, type, 0);

                int currentIndex = startIndex;
                if (CaptureRuntimeAsyncCallstack(buffer, ref currentIndex, ref state))
                {
                    // Patch frame count in the event buffer using the offset from CallstackHeader.
                    buffer[frameCountOffset] = state.Count;
                    Serializer.CommitAsyncEvent(context, ref eventBuffer, currentTimestamp, currentIndex);
                }
                else
                {
                    EmitAsyncCallstackSlowPath(context, ref eventBuffer, startIndex, currentIndex, eventID, id, type, ref state);
                }
            }

            [MethodImpl(MethodImplOptions.NoInlining)]
            private static void EmitAsyncCallstackSlowPath(AsyncThreadContext context, ref EventBuffer eventBuffer, int startIndex, int currentIndex, AsyncEventID eventID, ulong id, AsyncCallstackType type, ref CaptureRuntimeAsyncCallstackState state)
            {
                // Max callstack data that can fit in the buffer after flush.
                int maxCallstackBytes = Math.Min(byte.MaxValue * MaxAsyncMethodFrameSize, eventBuffer.Data.Length);

                byte[]? rentedArray = RentArray(maxCallstackBytes);
                if (rentedArray != null)
                {
                    int length = currentIndex - startIndex;
                    int callstackIndex = length;

                    Buffer.BlockCopy(eventBuffer.Data, startIndex, rentedArray, 0, length);
                    CaptureRuntimeAsyncCallstack(rentedArray, ref callstackIndex, ref state);

                    // Static callstack payload: type (1) + callstackId (1) + frameCount (1) + id (max 10 bytes compressed).
                    const int MaxStaticEventPayloadSize = sizeof(byte) + sizeof(byte) + sizeof(byte) + Serializer.MaxCompressedUInt64Size;

                    context.Flush();

                    // Write the callstack again.
                    int index = Serializer.BeginAsyncEvent(context, ref eventBuffer, context.LastEventTimestamp, eventID, MaxStaticEventPayloadSize + callstackIndex);
                    if (index != -1)
                    {
                        byte[] buffer = eventBuffer.Data;
                        CallstackHeader(buffer, ref index, id, type, state.Count);
                        Buffer.BlockCopy(rentedArray, 0, buffer, index, callstackIndex);
                        Serializer.CommitAsyncEvent(context, ref eventBuffer, context.LastEventTimestamp, index + callstackIndex);
                    }

                    ArrayPool<byte>.Shared.Return(rentedArray);
                }
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            private static int CallstackHeader(byte[] data, ref int index, ulong id, AsyncCallstackType type, byte callstackFrameCount)
            {
                // Callstack header layout: type (1 byte) + callstackId (1 byte, reserved for future use) + frameCount (1 byte) + id (max 10 bytes compressed).
                const int MaxCallstackHeaderSize = sizeof(byte) + sizeof(byte) + sizeof(byte) + Serializer.MaxCompressedUInt64Size;

                Span<byte> callstackHeaderSpan = data.AsSpan(index, MaxCallstackHeaderSize);
                int spanIndex = 0;

                callstackHeaderSpan[spanIndex++] = (byte)type;
                callstackHeaderSpan[spanIndex++] = 0; // Reserved callstack ID for future callstack interning.

                int frameCountOffset = index + spanIndex;
                callstackHeaderSpan[spanIndex++] = callstackFrameCount;

                spanIndex += Serializer.WriteCompressedUInt64(callstackHeaderSpan.Slice(spanIndex), id);
                index += spanIndex;

                return frameCountOffset;
            }

            private static byte[]? RentArray(int minimumLength)
            {
                byte[]? rentedArray = null;
                try
                {
                    rentedArray = ArrayPool<byte>.Shared.Rent(minimumLength);
                }
                catch
                {
                    //AsyncProfiler can't throw, return null if renting fails.
                }

                return rentedArray;
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static ulong GetTaskId(ref AsyncDispatcherInfo info)
        {
            if (info.CurrentTask != null)
            {
                return (ulong)info.CurrentTask.Id;
            }
            return 0;
        }
    }
}
