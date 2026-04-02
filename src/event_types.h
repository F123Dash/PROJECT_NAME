#ifndef EVENT_TYPES_H
#define EVENT_TYPES_H

// 🔹 Enum for all event types
enum class EventType {
    PACKET_GENERATE,
    PACKET_SEND,
    PACKET_RECEIVE,
    QUEUE_ENQUEUE,
    QUEUE_DEQUEUE,
    TIMEOUT,
    LINK_DOWN,
    LINK_UP
};

#endif // EVENT_TYPES_H
