// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System.Buffers.Binary;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.Tracing;
using System.Reflection;
using System.Linq;
using Microsoft.DotNet.XUnitExtensions;
using Xunit;

namespace System.Threading.Tasks.Tests
{
    public class AsyncProfilerTests
    {
        private static bool IsRuntimeAsyncSupported => PlatformDetection.IsRuntimeAsyncSupported;

        private const string AsyncProfilerEventSourceName = "System.Runtime.CompilerServices.AsyncProfilerEventSource";
        private const int BulkAsyncEventsId = 1;

        // AsyncProfilerEventSource Keywords matching the event source definition
        private const EventKeywords BulkResumeAsyncContext = (EventKeywords)0x1;
        private const EventKeywords BulkSuspendAsyncContext = (EventKeywords)0x2;
        private const EventKeywords BulkCompleteAsyncContext = (EventKeywords)0x4;
        private const EventKeywords BulkUnwindAsyncException = (EventKeywords)0x8;
        private const EventKeywords BulkResumeAsyncCallstack = (EventKeywords)0x10;
        private const EventKeywords BulkResumeAsyncMethod = (EventKeywords)0x20;
        private const EventKeywords BulkCompleteAsyncMethod = (EventKeywords)0x40;

        private const EventKeywords AllBulkKeywords =
            BulkResumeAsyncContext | BulkSuspendAsyncContext | BulkCompleteAsyncContext |
            BulkUnwindAsyncException | BulkResumeAsyncCallstack |
            BulkResumeAsyncMethod | BulkCompleteAsyncMethod;

        private const EventKeywords CoreKeywords =
            BulkResumeAsyncContext | BulkSuspendAsyncContext | BulkCompleteAsyncContext;

        private const EventKeywords MethodKeywords =
            BulkResumeAsyncMethod | BulkCompleteAsyncMethod;


        // Bulk event IDs matching AsyncProfiler.BulkEventID
        private const byte ResumeAsyncContext = 10;
        private const byte SuspendAsyncContext = 11;
        private const byte CompleteAsyncContext = 12;
        private const byte UnwindAsyncException = 13;
        private const byte ResumeAsyncCallstack = 14;
        private const byte ResumeAsyncMethod = 15;
        private const byte CompleteAsyncMethod = 16;
        private const byte ResetAsyncThreadContext = 17;
        private const byte ResetContinuationWrapperIndex = 18;

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task Func()
        {
            await Task.Yield();
        }

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task FuncThatThrows()
        {
            await Task.Yield();
            throw new InvalidOperationException("test exception");
        }

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task FuncChained()
        {
            await FuncInner();
        }

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task FuncInner()
        {
            await Task.Yield();
        }

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task OuterCatches()
        {
            try
            {
                await InnerThrows();
            }
            catch (InvalidOperationException)
            {
            }
            await Task.Yield();
        }

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task InnerThrows()
        {
            await Task.Yield();
            throw new InvalidOperationException("inner");
        }

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task DeepOuterCatches()
        {
            try
            {
                await DeepMiddle();
            }
            catch (InvalidOperationException)
            {
            }
        }

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task DeepMiddle()
        {
            await DeepInnerThrows();
        }

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task DeepInnerThrows()
        {
            await Task.Yield();
            throw new InvalidOperationException("deep inner");
        }

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task DeepUnhandledOuter()
        {
            await DeepUnhandledMiddle();
        }

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task DeepUnhandledMiddle()
        {
            await DeepUnhandledInnerThrows();
        }

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task DeepUnhandledInnerThrows()
        {
            await Task.Yield();
            throw new InvalidOperationException("deep unhandled");
        }

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task RecursiveFunc(int depth)
        {
            if (depth <= 0)
            {
                await Task.Yield();
                return;
            }
            await RecursiveFunc(depth - 1);
        }

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task WrapperTestA(List<(string MethodName, int WrapperSlot)> captures)
        {
            await WrapperTestB(captures);
            captures.Add((nameof(WrapperTestA), GetCurrentWrapperSlot(nameof(WrapperTestA))));
        }

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task WrapperTestB(List<(string MethodName, int WrapperSlot)> captures)
        {
            await WrapperTestC(captures);
            captures.Add((nameof(WrapperTestB), GetCurrentWrapperSlot(nameof(WrapperTestB))));
        }

        [System.Runtime.CompilerServices.RuntimeAsyncMethodGeneration(true)]
        static async Task WrapperTestC(List<(string MethodName, int WrapperSlot)> captures)
        {
            await Task.Yield();
            captures.Add((nameof(WrapperTestC), GetCurrentWrapperSlot(nameof(WrapperTestC))));
        }

        private static TestEventListener CreateListener(EventKeywords keywords)
        {
            var listener = new TestEventListener();
            listener.AddSource(AsyncProfilerEventSourceName, EventLevel.Informational, keywords);
            return listener;
        }

        private static void SendFlushCommand()
        {
            const int FlushBulkBuffersCommand = 1;
            foreach (EventSource source in EventSource.GetSources())
            {
                if (source.Name == AsyncProfilerEventSourceName)
                {
                    EventSource.SendCommand(source, (EventCommand)FlushBulkBuffersCommand, null);
                    return;
                }
            }
        }

        private static ulong GetCurrentOSThreadId()
        {
            return (ulong)typeof(Thread)
                .GetProperty("CurrentOSThreadId", BindingFlags.Static | BindingFlags.NonPublic)!
                .GetValue(null)!;
        }

        private static int GetCurrentWrapperSlot(string resumedMethodName)
        {
            var st = new StackTrace();
            for (int i = 0; i < st.FrameCount - 1; i++)
            {
                string? name = st.GetFrame(i)?.GetMethod()?.Name;
                if (name is not null && name.Contains(resumedMethodName))
                {
                    // The next frame should be the Continuation_Wrapper_N that dispatched this method.
                    string? wrapperName = st.GetFrame(i + 1)?.GetMethod()?.Name;
                    if (wrapperName is not null && wrapperName.StartsWith("Continuation_Wrapper_", StringComparison.Ordinal))
                    {
                        return int.Parse(wrapperName.Substring("Continuation_Wrapper_".Length));
                    }
                    return -1;
                }
            }
            return -1;
        }

        private delegate bool BulkEventVisitor(byte eventId, ReadOnlySpan<byte> buffer, ref int index);

        private delegate bool BulkEventVisitorWithTimestamp(byte eventId, long timestamp, ReadOnlySpan<byte> buffer, ref int index);

        private static void ParseBulkBuffer(ReadOnlySpan<byte> buffer, BulkEventVisitor visitor)
        {
            ParseBulkBuffer(buffer, (byte eventId, long _, ReadOnlySpan<byte> buf, ref int idx) =>
                visitor(eventId, buf, ref idx));
        }

        private static void ParseBulkBuffer(ReadOnlySpan<byte> buffer, BulkEventVisitorWithTimestamp visitor)
        {
            int index = 0;

            if (buffer.Length < 1 || buffer[index++] != 1)
                return;

            SkipCompressedUInt64(buffer, ref index);
            long baseTimestamp = (long)ReadCompressedUInt64(buffer, ref index);

            while (index < buffer.Length)
            {
                if (index + 2 > buffer.Length)
                    break;

                long delta = (long)ReadCompressedUInt64(buffer, ref index);
                baseTimestamp += delta;

                byte eventId = buffer[index++];

                if (!visitor(eventId, baseTimestamp, buffer, ref index))
                    break;
            }
        }

        private static bool SkipEventPayload(byte eventId, ReadOnlySpan<byte> buffer, ref int index)
        {
            switch (eventId)
            {
                case ResumeAsyncContext:
                case SuspendAsyncContext:
                case CompleteAsyncContext:
                case ResumeAsyncMethod:
                case CompleteAsyncMethod:
                case ResetAsyncThreadContext:
                case ResetContinuationWrapperIndex:
                    return true;
                case UnwindAsyncException:
                    ReadCompressedUInt32(buffer, ref index);
                    return true;
                case ResumeAsyncCallstack:
                    SkipCallstackPayload(buffer, ref index);
                    return true;
                default:
                    return false;
            }
        }

        private static void SkipCompressedUInt64(ReadOnlySpan<byte> buffer, ref int index)
        {
            while (index < buffer.Length && (buffer[index++] & 0x80) != 0) { }
        }

        private static uint ReadCompressedUInt32(ReadOnlySpan<byte> buffer, ref int index)
        {
            uint value = 0;
            int shift = 0;
            byte b;
            do
            {
                b = buffer[index++];
                value |= (uint)(b & 0x7F) << shift;
                shift += 7;
            } while ((b & 0x80) != 0);

            return value;
        }

        private static ulong ReadCompressedUInt64(ReadOnlySpan<byte> buffer, ref int index)
        {
            ulong value = 0;
            int shift = 0;
            byte b;
            do
            {
                b = buffer[index++];
                value |= (ulong)(b & 0x7F) << shift;
                shift += 7;
            } while ((b & 0x80) != 0);

            return value;
        }

        private static void SkipCallstackPayload(ReadOnlySpan<byte> buffer, ref int index)
        {
            ReadCallstackPayload(buffer, ref index, out _, out _);
        }

        private static void ReadCallstackPayload(ReadOnlySpan<byte> buffer, ref int index,
            out byte frameCount, out List<(ulong NativeIP, int State)> frames)
        {
            ReadCompressedUInt64(buffer, ref index);
            index++;
            frameCount = buffer[index++];
            frames = new List<(ulong, int)>(frameCount);

            if (frameCount == 0)
                return;

            ulong currentNativeIP = ReadCompressedUInt64(buffer, ref index);
            int state = ReadCompressedInt32(buffer, ref index);
            frames.Add((currentNativeIP, state));

            for (int i = 1; i < frameCount; i++)
            {
                long delta = ReadCompressedInt64(buffer, ref index);
                state = ReadCompressedInt32(buffer, ref index);
                currentNativeIP = (ulong)((long)currentNativeIP + delta);
                frames.Add((currentNativeIP, state));
            }
        }

        private static int ReadCompressedInt32(ReadOnlySpan<byte> buffer, ref int index)
        {
            uint encoded = ReadCompressedUInt32(buffer, ref index);
            return (int)((encoded >> 1) ^ (~(encoded & 1) + 1));
        }

        private static long ReadCompressedInt64(ReadOnlySpan<byte> buffer, ref int index)
        {
            ulong encoded = ReadCompressedUInt64(buffer, ref index);
            return (long)((encoded >> 1) ^ (~(encoded & 1) + 1));
        }

        private static ulong ParseOsThreadId(ReadOnlySpan<byte> buffer)
        {
            if (buffer.Length < 2 || buffer[0] != 1)
                return 0;

            int index = 1;
            return ReadCompressedUInt64(buffer, ref index);
        }

        private static List<byte> CollectBulkEventIds(ConcurrentQueue<EventWrittenEventArgs> events)
        {
            var allEventIds = new List<byte>();
            ForEachBulkPayload(events, buffer =>
            {
                ParseBulkBuffer(buffer, (byte eventId, ReadOnlySpan<byte> buf, ref int idx) =>
                {
                    allEventIds.Add(eventId);
                    return SkipEventPayload(eventId, buf, ref idx);
                });
            });
            return allEventIds;
        }

        private static HashSet<ulong> CollectOsThreadIds(ConcurrentQueue<EventWrittenEventArgs> events)
        {
            var threadIds = new HashSet<ulong>();
            ForEachBulkPayload(events, buffer =>
            {
                ulong tid = ParseOsThreadId(buffer);
                if (tid != 0)
                    threadIds.Add(tid);
            });
            return threadIds;
        }

        private static List<uint> CollectUnwindFrameCounts(ConcurrentQueue<EventWrittenEventArgs> events)
        {
            var frameCounts = new List<uint>();
            ForEachBulkPayload(events, buffer =>
            {
                ParseBulkBuffer(buffer, (byte eventId, ReadOnlySpan<byte> buf, ref int idx) =>
                {
                    if (eventId == UnwindAsyncException)
                    {
                        frameCounts.Add(ReadCompressedUInt32(buf, ref idx));
                        return true;
                    }
                    return SkipEventPayload(eventId, buf, ref idx);
                });
            });
            return frameCounts;
        }

        private static List<(byte FrameCount, List<(ulong NativeIP, int State)> Frames)> CollectCallstacks(
            ConcurrentQueue<EventWrittenEventArgs> events)
        {
            return CollectCallstacks(events, threadId: null);
        }

        private static List<(byte FrameCount, List<(ulong NativeIP, int State)> Frames)> CollectCallstacks(
            ConcurrentQueue<EventWrittenEventArgs> events, ulong? threadId)
        {
            var callstacks = new List<(byte, List<(ulong, int)>)>();
            ForEachBulkPayload(events, buffer =>
            {
                if (threadId.HasValue)
                {
                    ulong tid = ParseOsThreadId(buffer);
                    if (tid != threadId.Value)
                        return;
                }

                ParseBulkBuffer(buffer, (byte eventId, ReadOnlySpan<byte> buf, ref int idx) =>
                {
                    if (eventId == ResumeAsyncCallstack)
                    {
                        ReadCallstackPayload(buf, ref idx, out byte frameCount, out var frames);
                        callstacks.Add((frameCount, frames));
                        return true;
                    }
                    return SkipEventPayload(eventId, buf, ref idx);
                });
            });
            return callstacks;
        }

        private static (byte FrameCount, List<(ulong NativeIP, int State)> Frames)? FindCallstackAfterTimestamp(
            ConcurrentQueue<EventWrittenEventArgs> events, ulong threadId, long afterTimestamp)
        {
            (byte FrameCount, List<(ulong, int)> Frames)? best = null;
            long bestTimestamp = long.MaxValue;

            ForEachBulkPayload(events, buffer =>
            {
                ulong tid = ParseOsThreadId(buffer);
                if (tid != threadId)
                    return;

                ParseBulkBuffer(buffer, (byte eventId, long timestamp, ReadOnlySpan<byte> buf, ref int idx) =>
                {
                    if (eventId == ResumeAsyncCallstack)
                    {
                        ReadCallstackPayload(buf, ref idx, out byte frameCount, out var frames);
                        if (timestamp >= afterTimestamp && timestamp < bestTimestamp)
                        {
                            bestTimestamp = timestamp;
                            best = (frameCount, frames);
                        }
                        return true;
                    }
                    return SkipEventPayload(eventId, buf, ref idx);
                });
            });

            return best;
        }

        private delegate void BulkPayloadAction(ReadOnlySpan<byte> payload);

        private static void ForEachBulkPayload(ConcurrentQueue<EventWrittenEventArgs> events, BulkPayloadAction action)
        {
            foreach (var e in events)
            {
                if (e.EventId == BulkAsyncEventsId && e.Payload is { Count: >= 1 } && e.Payload[0] is byte[] rawPayload)
                {
                    action(rawPayload);
                }
            }
        }

        private static void RunScenarioAndFlush(Func<Task> scenario)
        {
            Task.Run(scenario).GetAwaiter().GetResult();
            SendFlushCommand();
        }

        private static void RunScenario(Func<Task> scenario)
        {
            Task.Run(scenario).GetAwaiter().GetResult();
        }

        private static ConcurrentQueue<EventWrittenEventArgs> CollectEvents(EventKeywords keywords, Action callback)
        {
            var events = new ConcurrentQueue<EventWrittenEventArgs>();
            using (var listener = CreateListener(keywords))
            {
                listener.RunWithCallback(events.Enqueue, () =>
                {
                    SendFlushCommand();
                    events.Clear();
                    callback();
                });
            }
            return events;
        }

        private static void AssertCallstackSimulationReachesZero(ConcurrentQueue<EventWrittenEventArgs> events)
        {
            var eventIds = CollectBulkEventIds(events);
            var frameCounts = CollectUnwindFrameCounts(events);
            var callstacks = CollectCallstacks(events);

            int stackDepth = 0;
            int unwindIdx = 0;
            int callstackIdx = 0;

            foreach (byte id in eventIds)
            {
                switch (id)
                {
                    case ResumeAsyncCallstack:
                        if (callstackIdx < callstacks.Count)
                            stackDepth = callstacks[callstackIdx++].FrameCount;
                        break;
                    case CompleteAsyncMethod:
                        if (stackDepth > 0)
                            stackDepth--;
                        break;
                    case UnwindAsyncException:
                        if (unwindIdx < frameCounts.Count)
                            stackDepth = Math.Max(0, stackDepth - (int)frameCounts[unwindIdx++]);
                        break;
                }
            }

            Assert.True(callstackIdx > 0, "Expected at least one ResumeAsyncCallstack event");
            Assert.Equal(0, stackDepth);
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_BulkEventsEmitted()
        {
            var events = CollectEvents(AllBulkKeywords, () =>
            {
                RunScenarioAndFlush(async () =>
                {
                    await Func();
                });
            });

            Assert.True(events.Count > 0, "Expected at least one BulkAsyncEvents event to be emitted");
            Assert.Contains(events, e => e.EventId == BulkAsyncEventsId);
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_SuspendResumeCompleteEvents()
        {
            var events = CollectEvents(CoreKeywords, () =>
            {
                RunScenarioAndFlush(async () =>
                {
                    // If not Yield here there won't be a SuspendAsyncContext.
                    // First call is a regular sync invocation (no continuation chain).
                    // Yield in Func will create an RuntimeAsyncTask with continuation chain
                    // and schedule on thread pool. When chain is resumed there will be
                    // ResumeAsyncContext and CompleteAsyncContext since the chain won't suspend again.
                    // The first Yield fixes that creating and schedule the RuntimeAsyncTask and Func
                    // will be called from the dispatch loop triggering the expected sequence of events.
                    await Task.Yield();
                    await Func();
                });
            });

            var eventIds = CollectBulkEventIds(events);

            Assert.Contains(ResumeAsyncContext, eventIds);
            Assert.Contains(SuspendAsyncContext, eventIds);
            Assert.Contains(CompleteAsyncContext, eventIds);
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_ResumeCompleteMethodEvents()
        {
            var events = CollectEvents(MethodKeywords, () =>
            {
                RunScenarioAndFlush(async () =>
                {
                    await FuncChained();
                });
            });

            var eventIds = CollectBulkEventIds(events);

            Assert.Contains(ResumeAsyncMethod, eventIds);
            Assert.Contains(CompleteAsyncMethod, eventIds);
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_UnhandledExceptionUnwind()
        {
            var events = CollectEvents(BulkUnwindAsyncException | CoreKeywords, () =>
            {
                // lambda -> DeepUnhandledOuter -> DeepUnhandledMiddle -> DeepUnhandledInnerThrows (4 levels).
                // No try/catch in the chain — UnwindToPossibleHandler returns null,
                // triggering the unhandled exception path which faults the task.
                // unwindedFrames starts at 1 (current) + walks 2 more continuations = 3.
                try
                {
                    RunScenario(async () =>
                    {
                        await DeepUnhandledOuter();
                    });
                }
                catch (InvalidOperationException)
                {
                }

                SendFlushCommand();
            });

            var eventIds = CollectBulkEventIds(events);
            var frameCounts = CollectUnwindFrameCounts(events);

            Assert.Contains(ResumeAsyncContext, eventIds);
            Assert.Contains(UnwindAsyncException, eventIds);
            Assert.Contains(CompleteAsyncContext, eventIds);

            Assert.NotEmpty(frameCounts);
            Assert.All(frameCounts, count => Assert.Equal(4u, count));
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_HandledExceptionUnwind()
        {
            var events = CollectEvents(BulkUnwindAsyncException | CoreKeywords, () =>
            {
                // DeepOuterCatches -> DeepMiddle -> DeepInnerThrows (3 levels).
                // DeepOuterCatches has try/catch — UnwindToPossibleHandler finds the handler.
                // unwindedFrames starts at 1 (current) + walks 1 to find handler = 2.
                RunScenarioAndFlush(async () =>
                {
                    await DeepOuterCatches();
                });
            });

            var eventIds = CollectBulkEventIds(events);
            var frameCounts = CollectUnwindFrameCounts(events);

            Assert.Contains(ResumeAsyncContext, eventIds);
            Assert.Contains(UnwindAsyncException, eventIds);
            Assert.Contains(CompleteAsyncContext, eventIds);

            Assert.NotEmpty(frameCounts);
            Assert.All(frameCounts, count => Assert.Equal(2u, count));
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_ResetAsyncThreadContextEvent()
        {
            var events = CollectEvents(CoreKeywords, () =>
            {
                RunScenarioAndFlush(async () =>
                {
                    await Func();
                });
            });

            var eventIds = CollectBulkEventIds(events);

            Assert.Contains(ResetAsyncThreadContext, eventIds);
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_NoEventsWhenDisabled()
        {
            // Run async work WITHOUT a listener attached
            Task.Run(async () =>
            {
                for (int i = 0; i < 50; i++)
                {
                    await Func();
                }
            }).GetAwaiter().GetResult();

            // Now attach listener and verify no stale events are emitted
            var events = CollectEvents(CoreKeywords, () =>
            {
                // Don't run any async work - just check nothing comes through from before
                Thread.Sleep(100);
            });

            // There may be a ResetAsyncThreadContext from the SyncPoint when keywords change,
            // but there should be no suspend/resume/complete events from the earlier work.
            var eventIds = CollectBulkEventIds(events);
            int contextEvents = eventIds.FindAll(id => id == ResumeAsyncContext || id == SuspendAsyncContext || id == CompleteAsyncContext).Count;

            Assert.Equal(0, contextEvents);
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_EventSequenceOrder()
        {
            var events = CollectEvents(CoreKeywords, () =>
            {
                RunScenarioAndFlush(async () =>
                {
                    // If not Yield here there won't be a SuspendAsyncContext.
                    // First call is a regular sync invocation (no continuation chain).
                    // Yield in Func will create an RuntimeAsyncTask with continuation chain
                    // and schedule on thread pool. When chain is resumed there will be
                    // ResumeAsyncContext and CompleteAsyncContext since the chain won't suspend again.
                    // The first Yield fixes that creating and schedule the RuntimeAsyncTask and Func
                    // will be called from the dispatch loop triggering the expected sequence of events.
                    await Task.Yield();
                    await Func();
                });
            });

            var eventIds = CollectBulkEventIds(events);
            var coreEvents = eventIds.FindAll(id => id == ResumeAsyncContext || id == SuspendAsyncContext || id == CompleteAsyncContext);

            Assert.Equal(ResumeAsyncContext, coreEvents[0]);
            Assert.Equal(SuspendAsyncContext, coreEvents[1]);
            Assert.Equal(ResumeAsyncContext, coreEvents[2]);
            Assert.Equal(CompleteAsyncContext, coreEvents[3]);
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_PeriodicTimerFlush()
        {
            var events = CollectEvents(CoreKeywords, () =>
            {
                // Run scenario — do NOT flush explicitly afterwards.
                RunScenario(async () =>
                {
                    await Func();
                });

                // Wait for the periodic flush timer (1s interval) to detect the idle
                // buffer and flush it automatically.
                Thread.Sleep(2000);
            });

            var eventIds = CollectBulkEventIds(events);
            int coreEventCount = eventIds.FindAll(id => id == ResumeAsyncContext || id == SuspendAsyncContext || id == CompleteAsyncContext).Count;

            Assert.True(coreEventCount > 0, "Expected periodic timer to flush bulk buffer with core lifecycle events");
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_MultiThreadFlush()
        {
            const int threadCount = 4;
            var events = CollectEvents(CoreKeywords, () =>
            {
                // Ensure enough thread pool threads are available for concurrent execution.
                ThreadPool.GetMinThreads(out int prevWorker, out int prevIO);
                ThreadPool.SetMinThreads(threadCount, prevIO);

                var tasks = new Task[threadCount];
                for (int i = 0; i < threadCount; i++)
                {
                    tasks[i] = Task.Run(async () =>
                    {
                        await Func();
                    });
                }

                Task.WhenAll(tasks).GetAwaiter().GetResult();
                ThreadPool.SetMinThreads(prevWorker, prevIO);
                SendFlushCommand();
            });

            var threadIds = CollectOsThreadIds(events);

            Assert.True(threadIds.Count > 1, $"Expected events from multiple threads, got {threadIds.Count} distinct OS thread ID(s)");
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_DeadThreadFlush()
        {
            var events = CollectEvents(CoreKeywords, () =>
            {
                // Spawn a dedicated thread that runs async work then exits.
                // Its thread-local bulk buffer becomes orphaned when the thread dies.
                var thread = new Thread(() =>
                {
                    RunScenario(async () =>
                    {
                        await Func();
                    });
                });

                thread.IsBackground = true;
                thread.Start();
                thread.Join(TimeSpan.FromSeconds(10));

                // Do NOT send a flush command.
                // Wait for the periodic flush timer to detect the dead thread
                // and flush its orphaned buffer.
                Thread.Sleep(2000);
            });

            var eventIds = CollectBulkEventIds(events);
            int coreEventCount = eventIds.FindAll(id => id == ResumeAsyncContext || id == SuspendAsyncContext || id == CompleteAsyncContext).Count;

            Assert.True(coreEventCount > 0, "Expected periodic timer to flush dead thread's bulk buffer");
        }

        private const EventKeywords CallstackKeywords =
            BulkResumeAsyncContext | BulkResumeAsyncCallstack | BulkCompleteAsyncContext |
            BulkCompleteAsyncMethod | BulkUnwindAsyncException;

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_CallstackEmittedOnResume()
        {
            var events = CollectEvents(CallstackKeywords, () =>
            {
                RunScenarioAndFlush(async () =>
                {
                    await Func();
                });
            });

            var callstacks = CollectCallstacks(events);

            Assert.NotEmpty(callstacks);
            Assert.All(callstacks, cs =>
            {
                Assert.True(cs.FrameCount > 0, "Expected at least one frame in callstack");
                Assert.True(cs.Frames[0].NativeIP != 0, "Expected non-zero NativeIP in first frame");
            });
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_CallstackDepthMatchesChain()
        {
            var events = CollectEvents(CallstackKeywords, () =>
            {
                // FuncChained -> FuncInner -> lambda: 3 levels deep after FuncInner yields.
                RunScenarioAndFlush(async () =>
                {
                    await FuncChained();
                });
            });

            var callstacks = CollectCallstacks(events);

            Assert.NotEmpty(callstacks);
            Assert.Contains(callstacks, cs => cs.FrameCount == 3);
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_CallstackSimulation_NormalCompletion()
        {
            var events = CollectEvents(CallstackKeywords, () =>
            {
                RunScenarioAndFlush(async () =>
                {
                    await FuncChained();
                });
            });

            AssertCallstackSimulationReachesZero(events);
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_CallstackSimulation_HandledException()
        {
            var events = CollectEvents(CallstackKeywords, () =>
            {
                // DeepOuterCatches -> DeepMiddle -> DeepInnerThrows: exception is caught
                // within the chain. Unwind pops 2 frames, execution resumes in outer.
                RunScenarioAndFlush(async () =>
                {
                    await DeepOuterCatches();
                });
            });

            AssertCallstackSimulationReachesZero(events);
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_CallstackSimulation_UnhandledException()
        {
            var events = CollectEvents(CallstackKeywords, () =>
            {
                // DeepUnhandledOuter -> DeepUnhandledMiddle -> DeepUnhandledInnerThrows:
                // no catch in the chain. Unwind pops all 3 frames, task faults.
                Task task = Task.Run(DeepUnhandledOuter);
                try
                {
                    task.GetAwaiter().GetResult();
                }
                catch (InvalidOperationException)
                {
                }
                SendFlushCommand();
            });

            AssertCallstackSimulationReachesZero(events);
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_WrapperIndexMatchesCallstack()
        {
            var captures = new List<(string MethodName, int WrapperSlot)>();
            ulong scenarioThreadId = 0;
            long scenarioTimestamp = 0;

            var events = CollectEvents(CallstackKeywords, () =>
            {
                // Capture a timestamp just before the scenario runs.
                // The callstack event closest after this timestamp on the
                // scenario thread is the one we want — simulating how a CPU
                // sampler would correlate a sample with a callstack.
                scenarioTimestamp = Stopwatch.GetTimestamp();

                // WrapperTestA -> WrapperTestB -> WrapperTestC.
                // Each method captures which Continuation_Wrapper_N dispatched it.
                RunScenarioAndFlush(async () =>
                {
                    await WrapperTestA(captures);
                    scenarioThreadId = GetCurrentOSThreadId();
                });
            });

            Assert.True(scenarioThreadId != 0, "Failed to capture scenario thread ID");
            Assert.True(captures.Count == 3, $"Expected 3 wrapper captures, got {captures.Count}");
            Assert.All(captures, c => Assert.True(c.WrapperSlot >= 0, $"{c.MethodName} did not find Continuation_Wrapper_N on stack (slot={c.WrapperSlot})"));

            int slotC = captures.First(c => c.MethodName == nameof(WrapperTestC)).WrapperSlot;
            int slotB = captures.First(c => c.MethodName == nameof(WrapperTestB)).WrapperSlot;
            int slotA = captures.First(c => c.MethodName == nameof(WrapperTestA)).WrapperSlot;

            Assert.Equal(slotC + 1, slotB);
            Assert.Equal(slotB + 1, slotA);

            var chainStack = FindCallstackAfterTimestamp(events, scenarioThreadId, scenarioTimestamp);

            Assert.True(chainStack.HasValue, "No callstack found after scenario timestamp on scenario thread");
            Assert.True(chainStack.Value.FrameCount == 4, $"Expected callstack with 4 frames, got {chainStack.Value.FrameCount}");

            MethodInfo getMethodFromIP = typeof(System.Diagnostics.StackFrame).GetMethod("GetMethodFromNativeIP", BindingFlags.Static | BindingFlags.NonPublic)!;
            Assert.NotNull(getMethodFromIP);

            var resolvedNames = new List<string>();
            foreach (var (nativeIP, _) in chainStack.Value.Frames)
            {
                var method = (MethodBase?)getMethodFromIP.Invoke(null, new object[] { (IntPtr)nativeIP });
                resolvedNames.Add(method?.Name ?? "<unknown>");
            }

            Assert.Equal(nameof(WrapperTestC), resolvedNames[slotC]);
            Assert.Equal(nameof(WrapperTestB), resolvedNames[slotB]);
            Assert.Equal(nameof(WrapperTestA), resolvedNames[slotA]);
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_WrapperIndexResetEmitted()
        {
            var events = CollectEvents(AllBulkKeywords, () =>
            {
                // Recursive chain 33 levels deep crosses the 32-slot boundary,
                // triggering at least one ResetContinuationWrapperIndex event.
                RunScenarioAndFlush(async () =>
                {
                    await RecursiveFunc(33);
                });
            });

            var eventIds = CollectBulkEventIds(events);

            Assert.Contains(ResetContinuationWrapperIndex, eventIds);
        }

        [ConditionalFact(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        public void RuntimeAsync_WrapperIndexNoResetUnder32()
        {
            var events = CollectEvents(AllBulkKeywords, () =>
            {
                // A shallow chain stays within the first 32 slots —
                // no reset event should be emitted.
                RunScenarioAndFlush(async () =>
                {
                    await RecursiveFunc(2);
                });
            });

            var eventIds = CollectBulkEventIds(events);

            Assert.DoesNotContain(ResetContinuationWrapperIndex, eventIds);
        }

        public static IEnumerable<object[]> KeywordGatekeepingData()
        {
            yield return new object[] { (long)BulkResumeAsyncContext, new byte[] { ResetAsyncThreadContext, ResumeAsyncContext } };
            yield return new object[] { (long)BulkSuspendAsyncContext, new byte[] { ResetAsyncThreadContext, SuspendAsyncContext } };
            yield return new object[] { (long)BulkCompleteAsyncContext, new byte[] { ResetAsyncThreadContext, CompleteAsyncContext } };
            yield return new object[] { (long)BulkUnwindAsyncException, new byte[] { ResetAsyncThreadContext, UnwindAsyncException } };
            yield return new object[] { (long)BulkResumeAsyncCallstack, new byte[] { ResetAsyncThreadContext, ResumeAsyncCallstack } };
            yield return new object[] { (long)BulkResumeAsyncMethod, new byte[] { ResetAsyncThreadContext, ResumeAsyncMethod } };
            yield return new object[] { (long)BulkCompleteAsyncMethod, new byte[] { ResetAsyncThreadContext, CompleteAsyncMethod } };
        }

        [ConditionalTheory(typeof(AsyncProfilerTests), nameof(IsRuntimeAsyncSupported))]
        [ActiveIssue("https://github.com/dotnet/runtime/issues/124072", typeof(PlatformDetection), nameof(PlatformDetection.IsInterpreter))]
        [MemberData(nameof(KeywordGatekeepingData))]
        public void RuntimeAsync_KeywordGatekeeping(long keywordValue, byte[] allowedEventIds)
        {
            EventKeywords kw = (EventKeywords)keywordValue;
            var allowed = new HashSet<byte>(allowedEventIds);

            var events = CollectEvents(kw, () =>
            {
                // Run a scenario that exercises all event types: resume, suspend,
                // complete, method events, callstacks, and exception unwinds.
                // Only the events matching the enabled keyword should be emitted.
                RunScenarioAndFlush(async () =>
                {
                    await OuterCatches();
                    await FuncChained();
                });
            });

            var eventIds = CollectBulkEventIds(events);
            var unexpected = eventIds.FindAll(id => !allowed.Contains(id));

            Assert.True(unexpected.Count == 0,
                $"Keyword 0x{(long)kw:X}: unexpected event IDs [{string.Join(", ", unexpected)}], " +
                $"allowed [{string.Join(", ", allowed)}]");
        }
    }
}
