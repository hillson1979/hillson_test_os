/**
 * @file qobject.cpp
 * @brief QObject implementation
 */

extern "C" {
#include "libuser_minimal.h"
}

#include "qobject.h"

// ============================================================
//  Helpers
// ============================================================
static char *strdup_simple(const char *s) {
    if (!s) return nullptr;
    unsigned int len = 0;
    while (s[len]) len++;
    char *d = new char[len + 1];
    for (unsigned int i = 0; i <= len; i++) d[i] = s[i];
    return d;
}

static int strcmp_simple(const char *a, const char *b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

// ============================================================
//  Constructor / Destructor
// ============================================================
QObject::QObject(QObject *parent, const char *name)
    : m_parent(nullptr)
    , m_firstChild(nullptr)
    , m_nextSibling(nullptr)
    , m_prevSibling(nullptr)
    , m_name(nullptr)
    , m_signals(nullptr)
{
    m_name = strdup_simple(name);
    if (parent) {
        setParent(parent);
    }
}

QObject::~QObject() {
    // 1. Delete all children recursively
    while (m_firstChild) {
        QObject *child = m_firstChild;
        m_firstChild = child->m_nextSibling;
        child->m_parent = nullptr;
        child->m_nextSibling = nullptr;
        child->m_prevSibling = nullptr;
        delete child;
    }

    // 2. Remove from parent's child list
    if (m_parent) {
        if (m_prevSibling) {
            m_prevSibling->m_nextSibling = m_nextSibling;
        } else {
            m_parent->m_firstChild = m_nextSibling;
        }
        if (m_nextSibling) {
            m_nextSibling->m_prevSibling = m_prevSibling;
        }
    }

    // 3. Free all signal entries and connections
    SignalEntry *se = m_signals;
    while (se) {
        SignalEntry *nextSE = se->next;
        Connection *conn = se->first;
        while (conn) {
            Connection *nextConn = conn->next;
            delete conn;
            conn = nextConn;
        }
        delete se;
        se = nextSE;
    }

    // 4. Free name
    if (m_name) delete[] m_name;
}

// ============================================================
//  Parent-child tree
// ============================================================
void QObject::setParent(QObject *parent) {
    if (m_parent) {
        if (m_prevSibling) {
            m_prevSibling->m_nextSibling = m_nextSibling;
        } else {
            m_parent->m_firstChild = m_nextSibling;
        }
        if (m_nextSibling) {
            m_nextSibling->m_prevSibling = m_prevSibling;
        }
    }

    m_parent = parent;
    m_prevSibling = nullptr;

    if (parent) {
        m_nextSibling = parent->m_firstChild;
        if (parent->m_firstChild) {
            parent->m_firstChild->m_prevSibling = this;
        }
        parent->m_firstChild = this;
    } else {
        m_nextSibling = nullptr;
    }
}

void QObject::setObjectName(const char *name) {
    if (m_name) delete[] m_name;
    m_name = strdup_simple(name);
}

// ============================================================
//  Signal / Slot
// ============================================================
QObject::SignalEntry *QObject::findOrCreateSignal(const char *name) {
    SignalEntry *se = m_signals;
    while (se) {
        if (strcmp_simple(se->name, name) == 0) return se;
        se = se->next;
    }
    se = new SignalEntry;
    se->name = name;
    se->first = nullptr;
    se->next = m_signals;
    m_signals = se;
    return se;
}

bool QObject::connect(QObject *sender, const char *signal,
                      QObject *receiver, SlotFunc slot,
                      void *userData) {
    if (!sender || !signal || !receiver || !slot) return false;

    SignalEntry *se = sender->findOrCreateSignal(signal);

    // Check for duplicate
    Connection *existing = se->first;
    while (existing) {
        if (existing->receiver == receiver && existing->slot == slot &&
            existing->userData == userData) {
            return false;
        }
        existing = existing->next;
    }

    Connection *conn = new Connection;
    conn->receiver = receiver;
    conn->slot     = slot;
    conn->userData = userData;
    conn->next     = se->first;
    se->first      = conn;

    return true;
}

void QObject::emitSignal(const char *signal, void *data) {
    SignalEntry *se = m_signals;
    while (se) {
        if (strcmp_simple(se->name, signal) == 0) {
            Connection *conn = se->first;
            while (conn) {
                // Use connection's userData if set, else emit-time data
                void *slotData = conn->userData ? conn->userData : data;
                conn->slot(this, slotData);
                conn = conn->next;
            }
            return;
        }
        se = se->next;
    }
}

int QObject::connectionCount() const {
    int count = 0;
    SignalEntry *se = m_signals;
    while (se) {
        Connection *conn = se->first;
        while (conn) {
            count++;
            conn = conn->next;
        }
        se = se->next;
    }
    return count;
}
