// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System.Buffers;
using System.Diagnostics;
using System.Diagnostics.Tracing;
using static System.Runtime.CompilerServices.AsyncProfilerEventSource;

namespace System.Runtime.CompilerServices
{
    internal static partial class AsyncProfiler
    {
        internal static partial class ResumeAsyncContext
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static ulong GetId(ref AsyncDispatcherInfo info)
            {
                if (info.CurrentTask != null)
                {
                    return (ulong)info.CurrentTask!.Id;
                }
                return 0;
            }

            public static void Resume(ref AsyncDispatcherInfo info)
            {
                AsyncThreadContext context = AsyncThreadContext.Acquire(ref info.AsyncProfilerInfo);

                try
                {
                    Resume(ref info, context, GetId(ref info), context.ActiveEventKeywords);
                }
                finally
                {
                    AsyncThreadContext.Release(context);
                }
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static void Resume(ref AsyncDispatcherInfo info, AsyncThreadContext context, ulong id, EventKeywords eventKeywords)
            {
                if (SyncPoint.Check(context))
                {
                    return;
                }

                if (IsEventKeywordEnabled.AnyBulkAsyncEvents(eventKeywords))
                {
                    long currentTimestamp = Stopwatch.GetTimestamp();
                    if (IsEventKeywordEnabled.BulkResumeAsyncContextEvent(eventKeywords))
                    {
                        BulkEvent(context, currentTimestamp);
                    }

                    if (IsEventKeywordEnabled.BulkResumeAsyncCallstackEvent(eventKeywords))
                    {
                        AsyncCallstack.BulkEvent(context, currentTimestamp, id, info.NextContinuation);
                    }
                }
            }
        }

        internal static partial class AsyncMethodException
        {
            public static void Unhandled(ref AsyncDispatcherInfo info, AsyncInstrumentation.Flags flags, uint unwindedFrames)
            {
                if (AsyncInstrumentation.IsEnabled.UnwindAsyncException(flags))
                {
                    AsyncMethodException.UnwindException(ref info.AsyncProfilerInfo, unwindedFrames);
                }

                if (AsyncInstrumentation.IsEnabled.CompleteAsyncContext(flags))
                {
                    CompleteAsyncContext.Complete(ref info.AsyncProfilerInfo);
                }
            }

            public static void Handled(ref AsyncDispatcherInfo info, AsyncInstrumentation.Flags flags, uint unwindedFrames)
            {
                if (AsyncInstrumentation.IsEnabled.UnwindAsyncException(flags))
                {
                    AsyncMethodException.UnwindException(ref info.AsyncProfilerInfo, unwindedFrames);
                }
            }
        }

        internal static partial class ContinuationWrapper
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static void InitInfo(ref Info info)
            {
                info.ContinuationTable = ref Unsafe.As<ContinuationWrapperTable, nint>(ref _continuationWrappers);
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

            private static ContinuationWrapperTable _continuationWrappers = InitContinuationWrappers();
        }

        private static partial class SyncPoint
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            private static unsafe void ResumeAsyncCallstacks(AsyncThreadContext context)
            {
                //Write recursivly all the resume async callstack events.
                AsyncDispatcherInfo* info = AsyncDispatcherInfo.t_current;
                if (info != null)
                {
                    ResumeRuntimeAsyncCallstacks(info, context);
                }

            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            private static unsafe void ResumeRuntimeAsyncCallstacks(AsyncDispatcherInfo* info, AsyncThreadContext context)
            {
                if (info != null)
                {
                    ResumeRuntimeAsyncCallstacks(info->Next, context);
                    ResumeAsyncContext.Resume(ref *info, context, ResumeAsyncContext.GetId(ref *info), Config.ActiveEventKeywords);
                }
            }
        }

        private static partial class AsyncCallstack
        {
            public struct CaptureRuntimeAsyncCallstackState
            {
                public Continuation Continuation;
                public byte Count;
            }

            public static bool BulkCaptureRuntimeAsyncCallstack(Span<byte> buffer, ref int index, ref CaptureRuntimeAsyncCallstackState state)
            {
                if (index > buffer.Length)
                {
                    return false;
                }

                ulong previousNativeIP = 0;
                ulong currentNativeIP = 0;
                byte maxAsyncCallstackLength = (byte)((buffer.Length - index) / BULK_ASYNC_METHOD_INFO_SIZE);

                unsafe
                {
                    currentNativeIP = (ulong)state.Continuation.ResumeInfo->DiagnosticIP;
                }

                BulkBuffer.Serializer.CompressedUInt64(buffer, ref index, currentNativeIP);
                BulkBuffer.Serializer.CompressedInt32(buffer, ref index, state.Continuation.State);
                state.Count++;

                state.Continuation = state.Continuation.Next!;
                while (state.Count < maxAsyncCallstackLength && state.Continuation != null)
                {
                    previousNativeIP = currentNativeIP;

                    unsafe
                    {
                        currentNativeIP = (ulong)state.Continuation.ResumeInfo->DiagnosticIP;
                    }

                    BulkBuffer.Serializer.CompressedInt64(buffer, ref index, (long)(currentNativeIP - previousNativeIP));
                    BulkBuffer.Serializer.CompressedInt32(buffer, ref index, state.Continuation.State);

                    state.Count++;
                    state.Continuation = state.Continuation.Next!;
                }

                return state.Continuation == null;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static void BulkEvent(AsyncThreadContext context, long currentTimestamp, ulong id, Continuation? asyncCallstack)
            {
                BulkEvent(context, currentTimestamp, BulkEventID.ResumeAsyncCallstack, id, AsyncType.Runtime, asyncCallstack);
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static void BulkEvent(AsyncThreadContext context, BulkEventID eventID, ulong id, AsyncType type, Continuation? asyncCallstack)
            {
                long currentTimestamp = Stopwatch.GetTimestamp();
                long delta = currentTimestamp - context.LastBulkEventTimestamp;
                BulkEvent(context, currentTimestamp, delta, eventID, id, type, asyncCallstack);
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static void BulkEvent(AsyncThreadContext context, long currentTimestamp, BulkEventID eventID, ulong id, AsyncType type, Continuation? asyncCallstack)
            {
                BulkEvent(context, currentTimestamp, currentTimestamp - context.LastBulkEventTimestamp, eventID, id, type, asyncCallstack);
            }
            public static void BulkEvent(AsyncThreadContext context, long currentTimestamp, long delta, BulkEventID eventID, ulong id, AsyncType type, Continuation? asyncCallstack)
            {
                CaptureRuntimeAsyncCallstackState state = default;
                Span<byte> stackBuffer = stackalloc byte[64 * BULK_ASYNC_METHOD_INFO_SIZE];
                Span<byte> buffer = stackBuffer;
                byte[]? rentedArray = null;
                int index = 0;

                ref BulkBuffer bulkBuffer = ref context.BulkBuffer;

                // Max callstack data that can fit in the bulk buffer after flush.
                // Accounts for: buffer header (~20 bytes), event header (~11 bytes),
                // callstack event envelope (id + type + frameCount = ~12 bytes), and reserved space.
                int maxCallstackBytes = Math.Min(
                    byte.MaxValue * BULK_ASYNC_METHOD_INFO_SIZE,
                    bulkBuffer.Data.Length);

                if (asyncCallstack != null)
                {
                    state.Continuation = asyncCallstack;
                    if (!BulkCaptureRuntimeAsyncCallstack(stackBuffer, ref index, ref state))
                    {
                        rentedArray = ArrayPool<byte>.Shared.Rent(maxCallstackBytes);
                        stackBuffer.Slice(0, index).CopyTo(rentedArray);
                        BulkCaptureRuntimeAsyncCallstack(rentedArray.AsSpan(0, maxCallstackBytes), ref index, ref state);
                        buffer = rentedArray;
                    }
                }
                else
                {
                    buffer = Array.Empty<byte>().AsSpan();
                }

                // id (max 10 bytes compressed) + type (1 byte) + index bytes.
                int maxEventSize = sizeof(ulong) + 2 + sizeof(byte) + index;

                if (BulkBuffer.Serializer.AsyncEventHeader(context, ref bulkBuffer, currentTimestamp, delta, eventID, maxEventSize))
                {
                    BulkBuffer.Serializer.Callstack(ref bulkBuffer, id, type, state.Count, buffer, index);
                }

                if (rentedArray != null)
                {
                    ArrayPool<byte>.Shared.Return(rentedArray);
                }
            }
        }
    }
}
