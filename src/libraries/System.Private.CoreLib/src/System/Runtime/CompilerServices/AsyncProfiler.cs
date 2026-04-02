// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System.Buffers.Binary;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.Tracing;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using static System.Runtime.CompilerServices.AsyncProfilerEventSource;

//TODO: Implement AsyncV1 async method support.
//TODO: ETW can use EVENT_CONTROL_CODE_CAPTURE_STATE to flush on session end, but we need to implement flush on EventPipe session end as well covering EventPipe+UserEvents.
//TODO: QPC is the major cost on bulk scenarios, ~ 15-20 ns per event. Possible to do direct call to __rdtsc, but it will still be ~10 ns per event.
//TODO: Deep callstacks are more expensive, investigate callstack folding as done on EventPipe to reduce serialization cost and size for identical resume callstacks.
//TODO: Emit a sync event to match clocks + additional metadata for bulk scenario.
//TODO: Maybe we would need some metadata on the ResumeAsyncCallstack around the dispatcher loop address, its generic so we won't be able to precompute all.

namespace System.Runtime.CompilerServices
{
    internal static partial class AsyncProfiler
    {
        internal enum BulkEventID : byte
        {
            ResumeAsyncContext = 10,
            SuspendAsyncContext = 11,
            CompleteAsyncContext = 12,
            UnwindAsyncException = 13,
            ResumeAsyncCallstack = 14,
            ResumeAsyncMethod = 15,
            CompleteAsyncMethod = 16,
            ResetAsyncThreadContext = 17,
            ResetContinuationWrapperIndex = 18
        }

        internal ref struct Info
        {
            public object? Context;
            public ref nint ContinuationTable;
            public uint ContinuationIndex;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void InitInfo(ref Info info)
        {
            info.Context = null;
            info.ContinuationIndex = 0;
            ContinuationWrapper.InitInfo(ref info);
        }

        internal static partial class Config
        {
            public static Lock ConfigLock { get; private set; } = new Lock();

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static bool Changed(AsyncThreadContext context) => context.ConfigRevision != Revision;

            public static void Update(EventLevel logLevel, EventKeywords eventKeywords)
            {
                lock (ConfigLock)
                {
                    Revision++;

                    ActiveEventKeywords = 0;
                    if (logLevel >= EventLevel.Informational)
                    {
                        ActiveEventKeywords = eventKeywords;
                    }

                    string? bulkBufferSizeEnv = System.Environment.GetEnvironmentVariable("DOTNET_AsyncProfilerEventSource_BulkBufferSize");
                    if (bulkBufferSizeEnv != null && int.TryParse(bulkBufferSizeEnv, out int bulkBufferSize) && bulkBufferSize >= 1024)
                    {
                        BulkBufferSize = bulkBufferSize;
                    }

                    if (IsEventKeywordEnabled.AnyBulkAsyncEvents(ActiveEventKeywords))
                    {
                        AsyncThreadContextCache.EnableFlushTimer();
                        AsyncThreadContextCache.DisableCleanupTimer();
                    }
                    else
                    {
                        AsyncThreadContextCache.DisableFlushTimer();
                        AsyncThreadContextCache.EnableCleanupTimer();
                    }

                    // Writer thread access both ActiveFlags and Revision without explicit acquire/release semantics,
                    // but Flags will be read before calling AcquireAsyncThreadContext that includes one volatile read
                    // acting as the load barrier for ActiveFlags and Revision.
                    Interlocked.MemoryBarrier();

                    UpdateFlags();
                }
            }

            private static void UpdateFlags()
            {
                AsyncInstrumentation.Flags flags = AsyncInstrumentation.Flags.Disabled;
                flags |= IsEventKeywordEnabled.BulkResumeAsyncContextEvent(ActiveEventKeywords) || IsEventKeywordEnabled.BulkResumeAsyncCallstackEvent(ActiveEventKeywords) ? AsyncInstrumentation.Flags.ResumeAsyncContext : 0;
                flags |= IsEventKeywordEnabled.BulkSuspendAsyncContextEvent(ActiveEventKeywords) ? AsyncInstrumentation.Flags.SuspendAsyncContext : 0;
                flags |= IsEventKeywordEnabled.BulkCompleteAsyncContextEvent(ActiveEventKeywords) ? AsyncInstrumentation.Flags.CompleteAsyncContext : 0;
                flags |= IsEventKeywordEnabled.BulkUnwindAsyncExceptionEvent(ActiveEventKeywords) ? AsyncInstrumentation.Flags.UnwindAsyncException : 0;
                flags |= IsEventKeywordEnabled.BulkResumeAsyncMethodEvent(ActiveEventKeywords) ? AsyncInstrumentation.Flags.ResumeAsyncMethod : 0;
                flags |= IsEventKeywordEnabled.BulkCompleteAsyncMethodEvent(ActiveEventKeywords) ? AsyncInstrumentation.Flags.CompleteAsyncMethod : 0;

                AsyncInstrumentation.UpdateAsyncProfilerFlags(flags);
            }

            public static void CaptureState()
            {
                AsyncThreadContextCache.Flush(true);
            }

            public static EventKeywords ActiveEventKeywords { get; private set; }

            public static uint Revision { get; private set; }

            // Use system page size as default bulk buffer size - 256 to cover for additional event headers.
            public static int BulkBufferSize { get; private set; } = Environment.SystemPageSize - 256;
        }

        internal struct BulkBuffer
        {
            public byte[] Data;

            public int Index;

            public int EventCount;

            public static class Serializer
            {
                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void Int32(Span<byte> buffer, ref int index, int value)
                {
                    UInt32(buffer, ref index, (uint)value);
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void Int32(byte[] buffer, ref int index, int value)
                {
                    UInt32(buffer, ref index, (uint)value);
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void CompressedInt32(Span<byte> buffer, ref int index, int value)
                {
                    CompressedUInt32(buffer, ref index, ZigzagEncodeInt32(value));
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void CompressedInt32(byte[] buffer, ref int index, int value)
                {
                    CompressedUInt32(buffer, ref index, ZigzagEncodeInt32(value));
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void UInt32(Span<byte> buffer, ref int index, uint value)
                {
                    Debug.Assert((uint)index <= (uint)(buffer.Length - 4));
                    UInt32(ref MemoryMarshal.GetReference(buffer), ref index, value);
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void UInt32(byte[] buffer, ref int index, uint value)
                {
                    Debug.Assert((uint)index <= (uint)(buffer.Length - 4));
                    UInt32(ref MemoryMarshal.GetArrayDataReference(buffer), ref index, value);
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                private static void UInt32(ref byte buffer, ref int index, uint value)
                {
                    Unsafe.WriteUnaligned(ref Unsafe.Add(ref buffer, index), value);
                    index += 4;
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void CompressedUInt32(Span<byte> buffer, ref int index, uint value)
                {
                    Debug.Assert((uint)index <= (uint)(buffer.Length - 5));
                    CompressedUInt32(ref MemoryMarshal.GetReference(buffer), ref index, value);
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void CompressedUInt32(byte[] buffer, ref int index, uint value)
                {
                    Debug.Assert((uint)index <= (uint)(buffer.Length - 5));
                    CompressedUInt32(ref MemoryMarshal.GetArrayDataReference(buffer), ref index, value);
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                private static void CompressedUInt32(ref byte buffer, ref int index, uint value)
                {
                    while (value > 0x7Fu)
                    {
                        Unsafe.Add(ref buffer, index++) = (byte)((uint)value | ~0x7Fu);
                        value >>= 7;
                    }
                    Unsafe.Add(ref buffer, index++) = (byte)value;
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void Int64(Span<byte> buffer, ref int index, long value)
                {
                    UInt64(buffer, ref index, (ulong)value);
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void Int64(byte[] buffer, ref int index, long value)
                {
                    UInt64(buffer, ref index, (ulong)value);
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void CompressedInt64(Span<byte> buffer, ref int index, long value)
                {
                    CompressedUInt64(buffer, ref index, ZigzagEncodeInt64(value));
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void CompressedInt64(byte[] buffer, ref int index, long value)
                {
                    CompressedUInt64(buffer, ref index, ZigzagEncodeInt64(value));
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void UInt64(Span<byte> buffer, ref int index, ulong value)
                {
                    Debug.Assert((uint)index <= (uint)(buffer.Length - 8));
                    UInt64(ref MemoryMarshal.GetReference(buffer), ref index, value);
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void UInt64(byte[] buffer, ref int index, ulong value)
                {
                    Debug.Assert((uint)index <= (uint)(buffer.Length - 8));
                    UInt64(ref MemoryMarshal.GetArrayDataReference(buffer), ref index, value);
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                private static void UInt64(ref byte buffer, ref int index, ulong value)
                {
                    Unsafe.WriteUnaligned(ref Unsafe.Add(ref buffer, index), value);
                    index += 8;
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void CompressedUInt64(Span<byte> buffer, ref int index, ulong value)
                {
                    Debug.Assert((uint)index <= (uint)(buffer.Length - 10));
                    CompressedUInt64(ref MemoryMarshal.GetReference(buffer), ref index, value);
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void CompressedUInt64(byte[] buffer, ref int index, ulong value)
                {
                    Debug.Assert((uint)index <= (uint)(buffer.Length - 10));
                    CompressedUInt64(ref MemoryMarshal.GetArrayDataReference(buffer), ref index, value);
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                private static void CompressedUInt64(ref byte buffer, ref int index, ulong value)
                {
                    while (value > 0x7Fu)
                    {
                        Unsafe.Add(ref buffer, index++) = (byte)((uint)value | ~0x7Fu);
                        value >>= 7;
                    }

                    Unsafe.Add(ref buffer, index++) = (byte)value;
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static uint ZigzagEncodeInt32(int value) => (uint)((value << 1) ^ (value >> 31));

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static ulong ZigzagEncodeInt64(long value) => (ulong)((value << 1) ^ (value >> 63));

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static void Header(AsyncThreadContext context, ref BulkBuffer bulkBuffer)
                {
                    byte[] buffer = bulkBuffer.Data;
                    ref int index = ref bulkBuffer.Index;

                    index = 0;
                    bulkBuffer.EventCount = 0;
                    context.LastBulkEventTimestamp = Stopwatch.GetTimestamp();

                    //Write header to buffer
                    Unsafe.Add(ref MemoryMarshal.GetArrayDataReference(buffer), index++) = 1; // Bulk version
                    CompressedUInt64(buffer, ref index, Thread.CurrentOSThreadId); // OS Thread ID
                    CompressedUInt64(buffer, ref index, (ulong)context.LastBulkEventTimestamp); // Timestamp
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static bool AsyncEventHeader(AsyncThreadContext context, ref BulkBuffer bulkBuffer, BulkEventID eventID, int maxEventSize)
                {
                    long currentTimestamp = Stopwatch.GetTimestamp();
                    long delta = currentTimestamp - context.LastBulkEventTimestamp;
                    return AsyncEventHeader(context, ref bulkBuffer, currentTimestamp, delta, eventID, maxEventSize);
                }

                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                public static bool AsyncEventHeader(AsyncThreadContext context, ref BulkBuffer bulkBuffer, long currentTimestamp, BulkEventID eventID, int maxEventSize)
                {
                    long delta = currentTimestamp - context.LastBulkEventTimestamp;
                    return AsyncEventHeader(context, ref bulkBuffer, currentTimestamp, delta, eventID, maxEventSize);
                }

                public static bool AsyncEventHeader(AsyncThreadContext context, ref BulkBuffer bulkBuffer, long currentTimestamp, long delta, BulkEventID eventID, int maxEventSize)
                {
                    const int fixedSize = sizeof(ulong) + 2 + sizeof(byte);

                    byte[] buffer = bulkBuffer.Data;
                    ref int index = ref bulkBuffer.Index;

                    if ((index + fixedSize + maxEventSize) <= buffer.Length && delta >= 0)
                    {
                        context.LastBulkEventTimestamp = currentTimestamp;
                    }
                    else
                    {
                        // Event is too big for buffer, drop it.
                        if (fixedSize + maxEventSize > buffer.Length)
                        {
                            return false;
                        }

                        context.Flush();
                        delta = 0;
                    }

                    CompressedUInt64(buffer, ref index, (ulong)delta); //Timestamp delta from last event
                    Unsafe.Add(ref MemoryMarshal.GetArrayDataReference(buffer), index++) = (byte)eventID; // eventID

                    bulkBuffer.EventCount++;
                    return true;
                }

                public static void Callstack(ref BulkBuffer bulkBuffer, ulong id, AsyncType type, byte callstackFrameCount, ReadOnlySpan<byte> callstackData, int callstackDataByteCount)
                {
                    byte[] buffer = bulkBuffer.Data;
                    ref int index = ref bulkBuffer.Index;

                    CompressedUInt64(buffer, ref index, id);

                    ref byte dst = ref MemoryMarshal.GetArrayDataReference(buffer);

                    Unsafe.Add(ref dst, index++) = (byte)type;
                    Unsafe.Add(ref dst, index++) = callstackFrameCount;

                    Unsafe.CopyBlockUnaligned(ref Unsafe.Add(ref dst, index), ref MemoryMarshal.GetReference(callstackData), (uint)callstackDataByteCount);
                    index += callstackDataByteCount;
                }
            }

#if DEBUG
            public static class Deserializer
            {
                public static int DumpBulkBuffer(ReadOnlySpan<byte> buffer)
                {
                    Debug.WriteLine("--- BulkAsyncEvents Buffer Dump ---");

                    int index = 0;

                    if ((uint)buffer.Length < 1)
                    {
                        Debug.WriteLine("Buffer too small.");
                        Debug.WriteLine("----------------------------------");
                        return index;
                    }

                    byte version = buffer[index++];
                    Debug.WriteLine($"BulkVersion: {version}");

                    if (version != 1)
                    {
                        Debug.WriteLine($"Unsupported bulk version: {version}");
                        Debug.WriteLine("----------------------------------");
                        return index;
                    }

                    CompressedUInt64(buffer, ref index, out ulong osThreadId);
                    CompressedUInt64(buffer, ref index, out ulong timestamp);

                    Debug.WriteLine($"OSThreadId: {osThreadId}");
                    Debug.WriteLine($"Timestamp: 0x{timestamp:X16}");

                    int eventCount = 0;
                    ulong currentTimestamp = timestamp;

                    while (index < buffer.Length)
                    {
                        if (index + 2 > buffer.Length)
                        {
                            Debug.WriteLine($"Trailing bytes: {buffer.Length - index} (incomplete entry header).");
                            break;
                        }

                        CompressedUInt64(buffer, ref index, out ulong delta);
                        currentTimestamp += delta;

                        BulkEventID eventId = (BulkEventID)buffer[index++];

                        Debug.WriteLine($"Entry[{eventCount}]: Timestamp=0x{currentTimestamp:X16}, EventId={eventId}");

                        int payloadStart = index;
                        try
                        {
                            index += eventId switch
                            {
                                BulkEventID.ResumeAsyncContext => ResumeAsyncContext.DumpEvent(),
                                BulkEventID.SuspendAsyncContext => SuspendAsyncContext.DumpEvent(),
                                BulkEventID.CompleteAsyncContext => CompleteAsyncContext.DumpEvent(),
                                BulkEventID.UnwindAsyncException => AsyncMethodException.DumpEvent(buffer.Slice(index)),
                                BulkEventID.ResumeAsyncCallstack => AsyncCallstack.DumpEvent(buffer.Slice(index)),
                                BulkEventID.ResumeAsyncMethod => ResumeAsyncMethod.DumpEvent(),
                                BulkEventID.CompleteAsyncMethod => CompleteAsyncMethod.DumpEvent(),
                                BulkEventID.ResetAsyncThreadContext => SyncPoint.DumpEvent(),
                                BulkEventID.ResetContinuationWrapperIndex => ContinuationWrapper.DumpEvent(),
                                _ => throw new InvalidOperationException($"Unknown eventId {eventId}."),
                            };
                        }
                        catch (Exception ex)
                        {
                            Debug.WriteLine($"  Failed decoding entry payload at offset {payloadStart}: {ex.GetType().Name}: {ex.Message}");
                            break;
                        }

                        eventCount++;
                    }

                    Debug.WriteLine($"TotalEntriesDecoded: {eventCount}");
                    Debug.WriteLine("----------------------------------");

                    return index;
                }

                public static void Int32(ReadOnlySpan<byte> buffer, ref int index, out int value)
                {
                    uint uValue;
                    UInt32(buffer, ref index, out uValue);
                    value = (int)uValue;
                }

                public static void CompressedInt32(ReadOnlySpan<byte> buffer, ref int index, out int value)
                {
                    uint uValue;
                    CompressedUInt32(buffer, ref index, out uValue);
                    value = ZigzagDecodeInt32(uValue);
                }

                public static void UInt32(ReadOnlySpan<byte> buffer, ref int index, out uint value)
                {
                    value = BinaryPrimitives.ReadUInt32LittleEndian(buffer.Slice(index));
                    index += 4;
                }

                public static void CompressedUInt32(ReadOnlySpan<byte> buffer, ref int index, out uint value)
                {
                    int shift = 0;
                    byte b;

                    value = 0;
                    do
                    {
                        b = buffer[index++];
                        value |= (uint)(b & 0x7F) << shift;
                        shift += 7;
                    } while ((b & 0x80) != 0);
                }

                public static void Int64(ReadOnlySpan<byte> buffer, ref int index, out long value)
                {
                    ulong uValue;
                    UInt64(buffer, ref index, out uValue);
                    value = (long)uValue;
                }

                public static void CompressedInt64(ReadOnlySpan<byte> buffer, ref int index, out long value)
                {
                    ulong uValue;
                    CompressedUInt64(buffer, ref index, out uValue);
                    value = ZigzagDecodeInt64(uValue);
                }

                public static void UInt64(ReadOnlySpan<byte> buffer, ref int index, out ulong value)
                {
                    value = BinaryPrimitives.ReadUInt64LittleEndian(buffer.Slice(index));
                    index += 8;
                }

                public static void CompressedUInt64(ReadOnlySpan<byte> buffer, ref int index, out ulong value)
                {
                    int shift = 0;
                    byte b;

                    value = 0;
                    do
                    {
                        b = buffer[index++];
                        value |= (ulong)(b & 0x7F) << shift;
                        shift += 7;
                    } while ((b & 0x80) != 0);
                }

                private static int ZigzagDecodeInt32(uint value) => (int)((value >> 1) ^ (~(value & 1) + 1));

                private static long ZigzagDecodeInt64(ulong value) => (long)((value >> 1) ^ (~(value & 1) + 1));
            }
#endif
        }

        internal sealed class AsyncThreadContext
        {
            public AsyncThreadContext()
            {
                _bulkBuffer.Data = Array.Empty<byte>();
            }

            private BulkBuffer _bulkBuffer;

            public long LastBulkEventTimestamp;

            public EventKeywords ActiveEventKeywords;

            public uint ConfigRevision;

            public volatile bool InUse;

            public volatile bool BlockContext;

            public ref BulkBuffer BulkBuffer
            {
                [MethodImpl(MethodImplOptions.AggressiveInlining)]
                get
                {
                    if (_bulkBuffer.Data.Length == 0)
                    {
                        _bulkBuffer.Data = new byte[Config.BulkBufferSize];
                        BulkBuffer.Serializer.Header(this, ref _bulkBuffer);
                    }

                    Debug.Assert(InUse || BlockContext);
                    return ref _bulkBuffer;
                }
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static AsyncThreadContext Acquire(ref Info info)
            {
                AsyncThreadContext context = Get(ref info);
                Debug.Assert(!context.InUse);

                context.InUse = true;
                if (context.BlockContext)
                {
                    context.InUse = false;
                    lock (AsyncThreadContextCache.CacheLock) {; }
                    context.InUse = true;
                }

                return context;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static void Release(AsyncThreadContext context)
            {
                Debug.Assert(context.InUse);
                context.InUse = false;
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static AsyncThreadContext Get()
            {
                AsyncThreadContext? context = t_asyncThreadContext;
                if (context != null)
                {
                    return context;
                }

                return Create();
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static AsyncThreadContext Get(ref Info info)
            {
                Debug.Assert(info.Context == null || info.Context is AsyncThreadContext);

                AsyncThreadContext? context = Unsafe.As<AsyncThreadContext?>(info.Context);
                if (context != null)
                {
                    return context;
                }

                context = Get();
                info.Context = t_asyncThreadContext;

                return context;
            }

            public void Reclaim()
            {
                Debug.Assert(InUse || BlockContext);

                _bulkBuffer.Data = Array.Empty<byte>();
                _bulkBuffer.Index = 0;
                _bulkBuffer.EventCount = 0;
            }

            public void Flush()
            {
                Debug.Assert(InUse || BlockContext);

                ref BulkBuffer bulkBuffer = ref BulkBuffer;

                if (bulkBuffer.EventCount == 0)
                {
                    return;
                }

                LogEvent(bulkBuffer.Data.AsSpan().Slice(0, bulkBuffer.Index));
                BulkBuffer.Serializer.Header(this, ref bulkBuffer);
            }

            private static void LogEvent(Span<byte> bulkBufferData)
            {
                Log.BulkAsyncEvents(bulkBufferData);
                DumpEvent(bulkBufferData);
            }

            private static AsyncThreadContext Create()
            {
                AsyncThreadContext context = new AsyncThreadContext();
                AsyncThreadContextCache.Add(context);
                t_asyncThreadContext = context;
                return context;
            }

#if DEBUG
            private static int DumpEvent(Span<byte> bulkBufferData)
            {
                return BulkBuffer.Deserializer.DumpBulkBuffer(bulkBufferData);
            }
#else
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            private static int DumpEvent(Span<byte> _) => 0;
#endif

            [ThreadStatic]
            private static AsyncThreadContext? t_asyncThreadContext;
        }

        internal static partial class ResumeAsyncContext
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static void BulkEvent(AsyncThreadContext context)
            {
                BulkBuffer.Serializer.AsyncEventHeader(context, ref context.BulkBuffer, BulkEventID.ResumeAsyncContext, 0);
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static void BulkEvent(AsyncThreadContext context, long currentTimestamp)
            {
                BulkBuffer.Serializer.AsyncEventHeader(context, ref context.BulkBuffer, currentTimestamp, BulkEventID.ResumeAsyncContext, 0);
            }

#if DEBUG
            public static int DumpEvent()
            {
                Debug.WriteLine("--- ResumeAsyncContext ---");
                Debug.WriteLine("----------------------------");
                return 0;
            }
#else
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static int DumpEvent() => 0;
#endif
        }

        internal static partial class SuspendAsyncContext
        {
            public static void Suspend(ref Info info)
            {
                AsyncThreadContext context = AsyncThreadContext.Acquire(ref info);

                try
                {
                    SyncPoint.Check(context);

                    EventKeywords eventKeywords = context.ActiveEventKeywords;
                    if (IsEventKeywordEnabled.BulkSuspendAsyncContextEvent(eventKeywords))
                    {
                        BulkEvent(context, Stopwatch.GetTimestamp());
                    }
                }
                finally
                {
                    AsyncThreadContext.Release(context);
                }
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static void BulkEvent(AsyncThreadContext context, long currentTimestamp)
            {
                BulkBuffer.Serializer.AsyncEventHeader(context, ref context.BulkBuffer, currentTimestamp, BulkEventID.SuspendAsyncContext, 0);
            }

#if DEBUG
            public static int DumpEvent()
            {
                Debug.WriteLine("--- SuspendAsyncContext ---");
                Debug.WriteLine("----------------------------");
                return 0;
            }
#else
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static int DumpEvent() => 0;
#endif
        }

        internal static partial class CompleteAsyncContext
        {
            public static void Complete(ref Info info)
            {
                AsyncThreadContext context = AsyncThreadContext.Acquire(ref info);

                try
                {
                    SyncPoint.Check(context);

                    EventKeywords eventKeywords = context.ActiveEventKeywords;
                    if (IsEventKeywordEnabled.BulkCompleteAsyncContextEvent(eventKeywords))
                    {
                        BulkEvent(context, Stopwatch.GetTimestamp());
                    }
                }
                finally
                {
                    AsyncThreadContext.Release(context);
                }
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static void BulkEvent(AsyncThreadContext context, long currentTimestamp)
            {
                BulkBuffer.Serializer.AsyncEventHeader(context, ref context.BulkBuffer, currentTimestamp, BulkEventID.CompleteAsyncContext, 0);
            }

#if DEBUG
            public static int DumpEvent()
            {
                Debug.WriteLine("--- CompleteAsyncContext ---");
                Debug.WriteLine("----------------------------");
                return 0;
            }
#else
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static int DumpEvent() => 0;
#endif
        }

        internal static partial class AsyncMethodException
        {
            public static void UnwindException(ref Info info, uint unwindedFrames)
            {
                AsyncThreadContext context = AsyncThreadContext.Acquire(ref info);

                try
                {
                    SyncPoint.Check(context);

                    EventKeywords eventKeywords = context.ActiveEventKeywords;
                    if (IsEventKeywordEnabled.BulkUnwindAsyncExceptionEvent(eventKeywords))
                    {
                        BulkEvent(context, Stopwatch.GetTimestamp(), unwindedFrames);
                    }
                }
                finally
                {
                    AsyncThreadContext.Release(context);
                }
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static void BulkEvent(AsyncThreadContext context, long currentTimestamp, uint unwindedFrames)
            {
                // unwinded frames (max 5 bytes compressed)
                const int maxEventSize = sizeof(uint) + 1;

                ref BulkBuffer bulkBuffer = ref context.BulkBuffer;

                if (BulkBuffer.Serializer.AsyncEventHeader(context, ref bulkBuffer, currentTimestamp, BulkEventID.UnwindAsyncException, maxEventSize))
                {
                    BulkBuffer.Serializer.CompressedUInt32(bulkBuffer.Data, ref bulkBuffer.Index, unwindedFrames);
                }
            }

#if DEBUG
            public static int DumpEvent(uint unwindedFrames)
            {
                Debug.WriteLine("--- UnwindAsyncException ---");
                Debug.WriteLine($"Unwinded Frames: {unwindedFrames}");
                Debug.WriteLine("----------------------------");
                return 0;
            }

            public static int DumpEvent(ReadOnlySpan<byte> buffer)
            {
                uint unwindedFrames;
                int index = 0;

                BulkBuffer.Deserializer.CompressedUInt32(buffer, ref index, out unwindedFrames);
                index += DumpEvent(unwindedFrames);

                return index;
            }
#else
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static int DumpEvent(uint _) => 0;
#endif
        }

        internal static partial class ResumeAsyncMethod
        {
            public static void Resume(ref Info info)
            {
                AsyncThreadContext context = AsyncThreadContext.Acquire(ref info);

                try
                {
                    SyncPoint.Check(context);

                    EventKeywords eventKeywords = context.ActiveEventKeywords;
                    if (IsEventKeywordEnabled.BulkResumeAsyncMethodEvent(eventKeywords))
                    {
                        BulkEvent(context);
                    }
                }
                finally
                {
                    AsyncThreadContext.Release(context);
                }
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static void BulkEvent(AsyncThreadContext context)
            {
                BulkBuffer.Serializer.AsyncEventHeader(context, ref context.BulkBuffer, BulkEventID.ResumeAsyncMethod, 0);
            }

#if DEBUG
            public static int DumpEvent()
            {
                Debug.WriteLine("--- ResumeAsyncMethod ---");
                Debug.WriteLine("----------------------------");
                return 0;
            }
#else
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static int DumpEvent() => 0;
#endif
        }

        internal static partial class CompleteAsyncMethod
        {
            public static void Complete(ref Info info)
            {
                AsyncThreadContext context = AsyncThreadContext.Acquire(ref info);

                try
                {
                    SyncPoint.Check(context);

                    EventKeywords eventKeywords = context.ActiveEventKeywords;
                    if (IsEventKeywordEnabled.BulkCompleteAsyncMethodEvent(eventKeywords))
                    {
                        BulkEvent(context);
                    }
                }
                finally
                {
                    AsyncThreadContext.Release(context);
                }
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static void BulkEvent(AsyncThreadContext context)
            {
                BulkBuffer.Serializer.AsyncEventHeader(context, ref context.BulkBuffer, BulkEventID.CompleteAsyncMethod, 0);
            }

#if DEBUG
            public static int DumpEvent()
            {
                Debug.WriteLine("--- CompleteAsyncMethod ---");
                Debug.WriteLine("----------------------------");
                return 0;
            }
#else
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static int DumpEvent() => 0;
#endif
        }

        internal static partial class ContinuationWrapper
        {
#pragma warning disable CA1823
            public const byte COUNT = 32;
            public const byte COUNT_MASK = COUNT - 1;
#pragma warning restore CA1823

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static void IncrementIndex(ref Info info)
            {
                info.ContinuationIndex++;
                if ((info.ContinuationIndex & COUNT_MASK) == 0)
                {
                    ResetIndex(ref info);
                }
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static void UnwindIndex(ref Info info, uint unwindedFrames)
            {
                uint oldIndex = info.ContinuationIndex;
                info.ContinuationIndex += unwindedFrames;

                if ((oldIndex & ~COUNT_MASK) != (info.ContinuationIndex & ~COUNT_MASK))
                {
                    ResetIndex(ref info);
                }
            }

            private static void ResetIndex(ref Info info)
            {
                AsyncThreadContext context = AsyncThreadContext.Acquire(ref info);

                try
                {
                    SyncPoint.Check(context);

                    EventKeywords eventKeywords = context.ActiveEventKeywords;
                    if (IsEventKeywordEnabled.AnyBulkAsyncEvents(eventKeywords))
                    {
                        BulkEvent(context);
                    }
                }
                finally
                {
                    AsyncThreadContext.Release(context);
                }
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            private static void BulkEvent(AsyncThreadContext context)
            {
                BulkBuffer.Serializer.AsyncEventHeader(context, ref context.BulkBuffer, BulkEventID.ResetContinuationWrapperIndex, 0);
            }

#if DEBUG
            public static int DumpEvent()
            {
                Debug.WriteLine("--- ResetContinuationWrapperIndex ---");
                Debug.WriteLine("----------------------------");
                return 0;
            }
#else
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static int DumpEvent() => 0;
#endif
        }

        private static partial class SyncPoint
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static bool Check(AsyncThreadContext context)
            {
                if (Config.Changed(context))
                {
                    ResetContext(context);
                    return true;
                }
                return false;
            }

            private static void ResetContext(AsyncThreadContext context)
            {
                context.Flush();

                context.ConfigRevision = Config.Revision;

                if (IsEventKeywordEnabled.AnyBulkAsyncEvents(Config.ActiveEventKeywords))
                {
                    BulkEvent(context);
                }

                context.ActiveEventKeywords = Config.ActiveEventKeywords;

                ResumeAsyncCallstacks(context);
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            private static void BulkEvent(AsyncThreadContext context)
            {
                BulkBuffer.Serializer.AsyncEventHeader(context, ref context.BulkBuffer, BulkEventID.ResetAsyncThreadContext, 0);
            }

#if DEBUG
            public static int DumpEvent()
            {
                Debug.WriteLine("--- ResetAsyncThreadContext ---");
                Debug.WriteLine("----------------------------");
                return 0;
            }
#else
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static int DumpEvent() => 0;
#endif
        }

        private static partial class AsyncCallstack
        {
#pragma warning disable CA1823
            private const int BULK_ASYNC_METHOD_INFO_SIZE = sizeof(ulong) + 2 + sizeof(int) + 1;
#pragma warning restore CA1823

#if DEBUG
            public static int DumpEvent(ReadOnlySpan<byte> buffer)
            {
                return DumpAsyncCallstackEvent(buffer);
            }

            private static int DumpAsyncCallstackEvent(ReadOnlySpan<byte> buffer)
            {
                ulong id;
                byte type;
                byte asyncCallstackLength;
                int index = 0;

                BulkBuffer.Deserializer.CompressedUInt64(buffer, ref index, out id);
                type = buffer[index++];
                asyncCallstackLength = buffer[index++];

                Debug.WriteLine($"--- ResumeAsyncCallstack ---");
                Debug.WriteLine($"ID: {id}");
                Debug.WriteLine($"Type: {type}");
                Debug.WriteLine($"Length: {asyncCallstackLength}");

                if (asyncCallstackLength == 0)
                {
                    return index;
                }

                ulong previousNativeIP;
                ulong currentNativeIP;
                int state;

                BulkBuffer.Deserializer.CompressedUInt64(buffer, ref index, out currentNativeIP);
                BulkBuffer.Deserializer.CompressedInt32(buffer, ref index, out state);

                OutputAsyncFrame(currentNativeIP, state, 0);

                for (int i = 1; i < asyncCallstackLength; i++)
                {
                    previousNativeIP = currentNativeIP;
                    BulkBuffer.Deserializer.CompressedInt64(buffer, ref index, out long nativeIPDelta);
                    BulkBuffer.Deserializer.CompressedInt32(buffer, ref index, out state);
                    currentNativeIP = previousNativeIP + (ulong)nativeIPDelta;
                    OutputAsyncFrame(currentNativeIP, state, i);
                }

                return index;
            }

#if NATIVEAOT || MONO
            internal static string ResolveAsyncMethodName(nint _)
            {
                return string.Empty;
            }
#else
            internal static string ResolveAsyncMethodName(nint nativeIP)
            {
                MethodBase? method = StackFrame.GetMethodFromNativeIP(nativeIP);
                return method?.Name ?? string.Empty;
            }
#endif

            private static void OutputAsyncFrame(ulong nativeIP, int state, int frameIndex)
            {
                string asyncMethodName = ResolveAsyncMethodName((nint)nativeIP);
                asyncMethodName = !string.IsNullOrEmpty(asyncMethodName) ? asyncMethodName : $"??";
                string nativeIPString = $"0x{nativeIP:X}";
                Debug.WriteLine($"  Frame {frameIndex}: AsyncMethod = {asyncMethodName}, NativeIP = {nativeIPString}, State = {state}");
            }
#endif
        }

        private static class IsEventKeywordEnabled
        {
            public static bool BulkResumeAsyncContextEvent(EventKeywords eventKeywords) => (eventKeywords & Keywords.BulkResumeAsyncContext) != 0;
            public static bool BulkSuspendAsyncContextEvent(EventKeywords eventKeywords) => (eventKeywords & Keywords.BulkSuspendAsyncContext) != 0;
            public static bool BulkCompleteAsyncContextEvent(EventKeywords eventKeywords) => (eventKeywords & Keywords.BulkCompleteAsyncContext) != 0;
            public static bool BulkUnwindAsyncExceptionEvent(EventKeywords eventKeywords) => (eventKeywords & Keywords.BulkUnwindAsyncException) != 0;
            public static bool BulkResumeAsyncCallstackEvent(EventKeywords eventKeywords) => (eventKeywords & Keywords.BulkResumeAsyncCallstack) != 0;
            public static bool BulkResumeAsyncMethodEvent(EventKeywords eventKeywords) => (eventKeywords & Keywords.BulkResumeAsyncMethod) != 0;
            public static bool BulkCompleteAsyncMethodEvent(EventKeywords eventKeywords) => (eventKeywords & Keywords.BulkCompleteAsyncMethod) != 0;
            public static bool AnyBulkAsyncEvents(EventKeywords eventKeywords) => (eventKeywords & BulkAsyncEventKeywords) != 0;
        }

        private static class AsyncThreadContextCache
        {
            public static Lock CacheLock { get; private set; } = new Lock();

            public static void Add(AsyncThreadContext context)
            {
                AsyncThreadContextHolder contextHolder = new AsyncThreadContextHolder(context, Thread.CurrentThread);
                lock (CacheLock)
                {
                    _cache.Add(contextHolder);
                }
            }

            public static void Flush(bool force)
            {
                lock (CacheLock)
                {
                    // Make sure all dead threads are flushed and removed from the cache.
                    for (int i = _cache.Count - 1; i >= 0; i--)
                    {
                        var contextHolder = _cache[i];
                        if (!contextHolder.OwnerThread.TryGetTarget(out Thread? target) || !target.IsAlive)
                        {
                            // Thread is dead, flush its buffer and remove from cache.
                            AsyncThreadContext context = contextHolder.Context;

                            Debug.Assert(!context.InUse);
                            context.InUse = true;

                            context.Flush();

                            context.Reclaim();
                            _cache.RemoveAt(i);

                            context.InUse = false;
                        }
                    }

                    // Look at live threads, only flush if forced or contexts that have been idle for 250 milliseconds.
                    long idleWriteTimestamp = Stopwatch.GetTimestamp() - (Stopwatch.Frequency / 4);

                    // Additionally, reclaim buffers for contexts that have been idle for 30 seconds to avoid keeping
                    // large buffers around indefinitely for threads that are no longer running async code.
                    long idleReclaimBufferTimestamp = Stopwatch.GetTimestamp() - Stopwatch.Frequency * 30;

                    foreach (var contextHolder in _cache)
                    {
                        AsyncThreadContext context = contextHolder.Context;

                        // Read LastBulkEventTimestamp without atomics, could cause teared reads but not critical.
                        long lastWriteTimestamp = context.LastBulkEventTimestamp;
                        if (force || lastWriteTimestamp < idleWriteTimestamp)
                        {
                            context.BlockContext = true;
                            while (context.InUse)
                            {
                                Thread.Yield();
                            }

                            Debug.Assert(!context.InUse);
                            context.Flush();

                            if (force || lastWriteTimestamp < idleReclaimBufferTimestamp)
                            {
                                context.Reclaim();
                            }

                            context.BlockContext = false;
                        }
                    }
                }
            }

            public static void EnableFlushTimer()
            {
                lock (CacheLock)
                {
                    _flushTimer ??= new Timer(PeriodicFlush, null, Timeout.Infinite, Timeout.Infinite);
                    _flushTimer.Change(ASYNC_THREAD_CONTEXT_CACHE_FLUSH_TIMER_INTERVAL_MS, Timeout.Infinite);
                }
            }

            public static void DisableFlushTimer()
            {
                lock (CacheLock)
                {
                    _flushTimer?.Change(Timeout.Infinite, Timeout.Infinite);
                }
            }

            public static void EnableCleanupTimer()
            {
                lock (CacheLock)
                {
                    _cleanupTimer ??= new Timer(Cleanup, null, Timeout.Infinite, Timeout.Infinite);
                    _cleanupTimer?.Change(ASYNC_THREAD_CONTEXT_CACHE_CLEANUP_TIMER_INTERVAL_MS, Timeout.Infinite);
                }
            }

            public static void DisableCleanupTimer()
            {
                lock (CacheLock)
                {
                    _cleanupTimer?.Change(Timeout.Infinite, Timeout.Infinite);
                }
            }

            private static void Cleanup(object? state)
            {
                _ = state;

                lock (CacheLock)
                {
                    Flush(true);

                    if (_cache.Count > 0)
                    {
                        // Restart cleanup timer.
                        _cleanupTimer?.Change(ASYNC_THREAD_CONTEXT_CACHE_CLEANUP_TIMER_INTERVAL_MS, Timeout.Infinite);
                    }
                }
            }

            private static void PeriodicFlush(object? state)
            {
                _ = state;

                lock (CacheLock)
                {
                    Flush(false);

                    if (IsEventKeywordEnabled.AnyBulkAsyncEvents(Config.ActiveEventKeywords))
                    {
                        // Restart flush timer.
                        _flushTimer?.Change(ASYNC_THREAD_CONTEXT_CACHE_FLUSH_TIMER_INTERVAL_MS, Timeout.Infinite);
                    }
                    else
                    {
                        // Start cleanup timer.
                        _cleanupTimer?.Change(ASYNC_THREAD_CONTEXT_CACHE_CLEANUP_TIMER_INTERVAL_MS, Timeout.Infinite);
                    }
                }
            }

            private sealed class AsyncThreadContextHolder
            {
                public AsyncThreadContextHolder(AsyncThreadContext context, Thread ownerThread)
                {
                    Context = context;
                    OwnerThread = new WeakReference<Thread>(ownerThread);
                }

                public AsyncThreadContext Context;
                public WeakReference<Thread> OwnerThread;
            }

            private const int ASYNC_THREAD_CONTEXT_CACHE_FLUSH_TIMER_INTERVAL_MS = 1000;
            private static Timer? _flushTimer;

            private const int ASYNC_THREAD_CONTEXT_CACHE_CLEANUP_TIMER_INTERVAL_MS = 30000;
            private static Timer? _cleanupTimer;

            private static List<AsyncThreadContextHolder> _cache = new List<AsyncThreadContextHolder>();
        }

#if MONO
        internal static partial class Config
        {
            private static void UpdateFlags()
            {
            }
        }

        internal static partial class ContinuationWrapper
        {
            public static void InitInfo(ref Info info)
            {
                info.ContinuationIndex = 0;
                info.ContinuationTable = ref _dummyContinuationTable;
            }

            private static nint _dummyContinuationTable;
        }

        private static partial class SyncPoint
        {
            private static unsafe void ResumeAsyncCallstacks(AsyncThreadContext _)
            {
            }
        }
#endif
    }
}
