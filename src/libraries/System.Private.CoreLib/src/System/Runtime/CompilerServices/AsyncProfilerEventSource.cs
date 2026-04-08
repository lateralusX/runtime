// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Diagnostics.Tracing;

namespace System.Runtime.CompilerServices
{
    /// <summary>Provides an event source for tracing async execution.</summary>
    [EventSource(
        Name = "System.Runtime.CompilerServices.AsyncProfilerEventSource",
        Guid = "742BD0FE-9200-4DE2-9059-576F35EBEB62"
        )]
    internal sealed partial class AsyncProfilerEventSource : EventSource
    {
        private const string EventSourceSuppressMessage = "Parameters to this method are primitive and are trimmer safe";

        public static readonly AsyncProfilerEventSource Log = new AsyncProfilerEventSource();

        // Bulk Core event mask: 0x1E
        // BulkSuspendAsyncContext (0x2), BulkCompleteAsyncContext (0x4), BulkUnwindAsyncException (0x8), BulkResumeAsyncCallstack (0x10)

        public static class Keywords // this name is important for EventSource
        {
            public const EventKeywords BulkCreateAsyncContext = (EventKeywords)0x1;
            public const EventKeywords BulkResumeAsyncContext = (EventKeywords)0x2;
            public const EventKeywords BulkSuspendAsyncContext = (EventKeywords)0x4;
            public const EventKeywords BulkCompleteAsyncContext = (EventKeywords)0x8;
            public const EventKeywords BulkUnwindAsyncException = (EventKeywords)0x10;
            public const EventKeywords BulkCreateAsyncCallstack = (EventKeywords)0x20;
            public const EventKeywords BulkResumeAsyncCallstack = (EventKeywords)0x40;
            public const EventKeywords BulkResumeAsyncMethod = (EventKeywords)0x80;
            public const EventKeywords BulkCompleteAsyncMethod = (EventKeywords)0x100;
        }

        public const EventKeywords BulkAsyncEventKeywords =
            Keywords.BulkCreateAsyncContext |
            Keywords.BulkResumeAsyncContext |
            Keywords.BulkSuspendAsyncContext |
            Keywords.BulkCompleteAsyncContext |
            Keywords.BulkCreateAsyncCallstack |
            Keywords.BulkResumeAsyncCallstack |
            Keywords.BulkUnwindAsyncException |
            Keywords.BulkResumeAsyncMethod |
            Keywords.BulkCompleteAsyncMethod;

        public static class Tasks
        {
            public const EventTask BulkAsyncEvents = (EventTask)1;
            public const EventTask AsyncEventsMetadata = (EventTask)2;
        }

        [Flags]
        public enum AsyncCallstackType : byte
        {
            Compiler = 0x1,
            Runtime = 0x2,
            Cached = 0x80
        }

        public const int FlushBulkBuffersCommand = 1;

        //----------------------- Event IDs (must be unique) -----------------------
        // IDs 10 - 20 are reserved for future use and matches AsyncProfiler.BulkEventID.
        public const int BULK_ASYNC_EVENTS_ID = 1;
        public const int ASYNC_EVENTS_METADATA_ID = 2;

        //-----------------------------------------------------------------------------------
        //
        // Events
        //
        [Event(
            BULK_ASYNC_EVENTS_ID,
            Task = Tasks.BulkAsyncEvents,
            Version = 1,
            Opcode = EventOpcode.Info,
            Level = EventLevel.Informational,
            Keywords = BulkAsyncEventKeywords,
            Message = "")]
        public void BulkAsyncEvents(byte[] buffer)
        {
            BulkAsyncEvents(buffer.AsSpan());
        }

        [NonEvent]
        [UnconditionalSuppressMessage("ReflectionAnalysis", "IL2026:UnrecognizedReflectionPattern", Justification = EventSourceSuppressMessage)]
        public void BulkAsyncEvents(ReadOnlySpan<byte> buffer)
        {
            unsafe
            {
                fixed (byte* pBuffer = buffer)
                {
                    int length = buffer.Length;
                    EventData* eventPayload = stackalloc EventData[2];
                    eventPayload[0].Size = sizeof(int);
                    eventPayload[0].DataPointer = ((IntPtr)(&length));
                    eventPayload[0].Reserved = 0;
                    eventPayload[1].Size = sizeof(byte) * length;
                    eventPayload[1].DataPointer = ((IntPtr)pBuffer);
                    eventPayload[1].Reserved = 0;
                    WriteEventCore(BULK_ASYNC_EVENTS_ID, 2, eventPayload);
                }
            }
        }

        [Event(
            ASYNC_EVENTS_METADATA_ID,
            Task = Tasks.AsyncEventsMetadata,
            Version = 1,
            Opcode = EventOpcode.Info,
            Level = EventLevel.Informational,
            Keywords = BulkAsyncEventKeywords,
            Message = "")]
        public void AsyncEventsMetadata(long qpc, long qpcFrequency, byte[] continuationWrapperIPs)
        {
            throw new NotImplementedException("This method is only for EventSource manifest generation and should not be called directly.");
        }

        [NonEvent]
        [UnconditionalSuppressMessage("ReflectionAnalysis", "IL2026:UnrecognizedReflectionPattern", Justification = EventSourceSuppressMessage)]
        public void AsyncEventsMetadata(long qpc, long qpcFrequency, long[] continuationWrapperIPs)
        {
            unsafe
            {
                fixed (long* cwIPs = continuationWrapperIPs)
                {
                    int length = continuationWrapperIPs.Length * sizeof(long);
                    EventData* eventPayload = stackalloc EventData[4];
                    eventPayload[0].Size = sizeof(long);
                    eventPayload[0].DataPointer = ((IntPtr)(&qpc));
                    eventPayload[0].Reserved = 0;
                    eventPayload[1].Size = sizeof(long);
                    eventPayload[1].DataPointer = ((IntPtr)(&qpcFrequency));
                    eventPayload[1].Reserved = 0;
                    eventPayload[2].Size = sizeof(int);
                    eventPayload[2].DataPointer = ((IntPtr)(&length));
                    eventPayload[2].Reserved = 0;
                    eventPayload[3].Size = length;
                    eventPayload[3].DataPointer = ((IntPtr)cwIPs);
                    eventPayload[3].Reserved = 0;
                    WriteEventCore(ASYNC_EVENTS_METADATA_ID, 4, eventPayload);
                }
            }
        }

        /// <summary>
        /// Get callbacks when the ETW sends us commands`
        /// </summary>
        protected override void OnEventCommand(EventCommandEventArgs command)
        {
            if (command.Command == (EventCommand)FlushBulkBuffersCommand || command.Command == EventCommand.SendManifest)
            {
                AsyncProfiler.Config.CaptureState();
                return;
            }

            AsyncProfiler.Config.Update(m_level, m_matchAnyKeyword);
        }
    }
}
