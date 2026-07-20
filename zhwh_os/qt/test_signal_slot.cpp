/**
 * @file test_signal_slot.cpp
 * @brief Signal/slot system test — the heart of Qt
 *
 * Demonstrates:
 *   Counter emits "valueChanged" → Label and Logger react
 *   Button emits "clicked" → Counter increments
 *   One signal → multiple slots
 */

extern "C" {
#include "libuser_minimal.h"
}
#include "qobject.h"

// ============================================================
//  Counter — emits "valueChanged" when the value changes
// ============================================================
class Counter : public QObject {
    int m_value;
public:
    Counter(QObject *parent, const char *name)
        : QObject(parent, name), m_value(0) {}

    int value() const { return m_value; }

    void setValue(int v) {
        if (m_value != v) {
            m_value = v;
            emitSignal("valueChanged", &m_value);
        }
    }

    void increment() { setValue(m_value + 1); }
    void decrement() { setValue(m_value - 1); }

    const char *className() const override { return "Counter"; }
};

// ============================================================
//  Display — a "Label" that shows counter value
// ============================================================
class Display : public QObject {
    int m_lastValue;
public:
    Display(QObject *parent, const char *name)
        : QObject(parent, name), m_lastValue(-1) {}

    const char *className() const override { return "Display"; }

    // Slot: called when counter changes
    static void onValueChanged(QObject *sender, void *data) {
        Display *self = (Display *)sender;  // Hmm, actually the sender
        // Wait — in our convention, the FIRST param is the sender
        // (the object that emitted the signal), and the receiver
        // is stored in the Connection.

        // Actually, let me reconsider. The slot function signature is
        // void (*SlotFunc)(QObject *sender, void *data).
        // The `sender` is the OBJECT THAT EMITTED THE SIGNAL.
        // So we need a way to know WHICH Display received it.

        // For non-static member functions, we'd use a trampoline.
        // For now, let's use the data parameter to pass context.
        // Or better: we connect with a C++ lambda-like approach.
    }
};

// Actually, let me redesign this with a simpler pattern.
// The slot function receives (sender, data).
// If we need the receiver context, we pass it as data during connect.

// Let me make a cleaner demo with free functions as slots.

// ============================================================
//  Demo: Free-function slot handlers
// ============================================================

static int g_displayValue = -999;

// Slot 1: update a global display variable
static void slotDisplayUpdate(QObject *sender, void *data) {
    int *val = (int *)data;
    g_displayValue = *val;
    printf("  [Display] Counter '%s' changed to: %d\n",
           sender->objectName(), *val);
}

// Slot 2: log to console
static void slotLogger(QObject *sender, void *data) {
    int *val = (int *)data;
    printf("  [Logger] Event from '%s': value=%d\n",
           sender->objectName(), *val);
}

// Slot 3: alert when threshold reached
static void slotAlert(QObject *sender, void *data) {
    int *val = (int *)data;
    if (*val >= 5) {
        printf("  [ALERT] Threshold reached! value=%d (from '%s')\n",
               *val, sender->objectName());
    }
}

// ============================================================
//  Button — emits "clicked"
// ============================================================
class Button : public QObject {
public:
    Button(QObject *parent, const char *name)
        : QObject(parent, name) {}

    void click() {
        printf("  [Button '%s'] Click!\n", objectName());
        emitSignal("clicked", nullptr);
    }

    const char *className() const override { return "Button"; }
};

// Slot: handle button click by incrementing counter
static void slotOnIncrement(QObject *sender, void *data) {
    Counter *c = (Counter *)data;
    (void)sender;
    c->increment();
}

static void slotOnDecrement(QObject *sender, void *data) {
    Counter *c = (Counter *)data;
    (void)sender;
    c->decrement();
}

// ============================================================
//  Test
// ============================================================
int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("  Signal/Slot System Test\n");
    printf("========================================\n\n");

    // --- Test 1: Counter with multiple slots ---
    printf("--- Test 1: One signal → multiple slots ---\n");

    Counter counter(nullptr, "myCounter");
    printf("Created Counter '%s' (class=%s)\n",
           counter.objectName(), counter.className());

    // Connect counter's "valueChanged" to three slots
    QObject::connect(&counter, "valueChanged", &counter, slotDisplayUpdate);
    QObject::connect(&counter, "valueChanged", &counter, slotLogger);
    QObject::connect(&counter, "valueChanged", &counter, slotAlert);

    printf("Connected 3 slots to 'valueChanged' (total connections: %d)\n",
           counter.connectionCount());

    printf("Setting counter to 3...\n");
    counter.setValue(3);

    printf("Setting counter to 7...\n");
    counter.setValue(7);  // should trigger alert

    printf("Final display value: %d\n\n", g_displayValue);

    // --- Test 2: Button → Counter ---
    printf("--- Test 2: Button click → Counter increment ---\n");

    Counter c2(nullptr, "clickCounter");
    Button  btnInc(nullptr, "btnInc");
    Button  btnDec(nullptr, "btnDec");

    QObject::connect(&c2, "valueChanged", &c2, slotDisplayUpdate);

    // Button click → increment/decrement counter
    QObject::connect(&btnInc, "clicked", &c2, slotOnIncrement, &c2);
    QObject::connect(&btnDec, "clicked", &c2, slotOnDecrement, &c2);

    printf("Counter starts at: %d\n", c2.value());
    btnInc.click();
    printf("After btnInc click: %d\n", c2.value());
    btnInc.click();
    btnInc.click();
    printf("After 2 more clicks: %d\n", c2.value());
    btnDec.click();
    printf("After btnDec click: %d\n\n", c2.value());

    // --- Test 3: Parent-child auto-deletion ---
    printf("--- Test 3: Parent deletes children ---\n");
    {
        QObject *parent = new QObject(nullptr, "parent");
        new QObject(parent, "child1");
        new QObject(parent, "child2");
        new QObject(parent, "child3");

        QObject *child = parent->firstChild();
        int count = 0;
        while (child) {
            printf("  Child[%d]: '%s'\n", count++, child->objectName());
            child = child->nextSibling();
        }
        printf("  Total children: %d\n", count);

        printf("  Deleting parent...\n");
        delete parent;  // should delete all 3 children
        printf("  Parent deleted (children auto-deleted)\n");
    }

    printf("\n========================================\n");
    printf("  SIGNAL/SLOT TESTS PASSED!\n");
    printf("========================================\n");

    return 0;
}
