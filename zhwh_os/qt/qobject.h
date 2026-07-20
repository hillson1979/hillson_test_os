/**
 * @file qobject.h
 * @brief QObject — the base of the Qt-style object system
 *
 * Features:
 *  - Parent-child ownership tree (parent deletes children)
 *  - Runtime signal/slot (no MOC required — signal names are strings)
 *  - Object naming for debugging
 *
 * Signal/slot usage:
 *   // Pattern A: data flows with the emit
 *   QObject::connect(&counter, "valueChanged", &counter, slotOnChange);
 *   counter.emitSignal("valueChanged", &newValue);
 *
 *   // Pattern B: fixed context bound at connect-time
 *   QObject::connect(&btn, "clicked", &counter, slotOnClick, &counter);
 *   btn.emitSignal("clicked");
 *
 * Slot signature:  void mySlot(QObject *sender, void *data)
 *   sender = the object that emitted the signal
 *   data   = userData from connect (if set) or data from emitSignal
 */

#ifndef QOBJECT_H
#define QOBJECT_H

// Slot function type
typedef void (*SlotFunc)(class QObject *sender, void *data);

class QObject {
public:
    explicit QObject(QObject *parent = nullptr, const char *name = "");
    virtual ~QObject();

    // --- Parent-child tree ---
    QObject *parent() const { return m_parent; }
    void setParent(QObject *parent);

    QObject *firstChild() const { return m_firstChild; }
    QObject *nextSibling() const { return m_nextSibling; }

    // --- Naming ---
    const char *objectName() const { return m_name; }
    void setObjectName(const char *name);

    // --- RTTI lite ---
    virtual const char *className() const { return "QObject"; }

    // --- Signal / Slot ---
    // Connect sender's signal to a slot function.
    // If userData != nullptr, it is passed to the slot instead of emit-time data.
    static bool connect(QObject *sender, const char *signal,
                        QObject *receiver, SlotFunc slot,
                        void *userData = nullptr);

    // Emit a signal — calls all connected slots synchronously.
    void emitSignal(const char *signal, void *data = nullptr);

    int connectionCount() const;

private:
    struct Connection {
        QObject    *receiver;
        SlotFunc    slot;
        void       *userData;   // bound context (overrides emit-time data)
        Connection *next;
    };

    struct SignalEntry {
        const char  *name;
        Connection  *first;
        SignalEntry *next;
    };

    SignalEntry *findOrCreateSignal(const char *name);

    QObject     *m_parent;
    QObject     *m_firstChild;
    QObject     *m_nextSibling;
    QObject     *m_prevSibling;
    char        *m_name;
    SignalEntry *m_signals;

    QObject(const QObject &);
    QObject &operator=(const QObject &);
};

#endif // QOBJECT_H
