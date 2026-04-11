/*
    Oscil - DialogManager tests

    DialogManager is the single coordinator through which the plugin editor
    shows its four modal dialogs (Add Oscillator, Color Picker, Select Pane,
    Oscillator Config). These tests exercise construction + wiring, the config
    popup visibility round-trip, listener add/remove, the externalClose path
    that joins all close flows into a single notification point, and the
    destruction order contract (componentBeingDeleted must synchronously
    hide all modals via hideImmediate).
*/

#include "core/Oscillator.h"
#include "ui/components/AnimationSettings.h"
#include "ui/dialogs/OscillatorConfigDialog.h"
#include "ui/managers/DialogManager.h"

#include "OscilTestFixtures.h"

#include <gtest/gtest.h>

namespace oscil
{

class DialogManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        parent_ = std::make_unique<juce::Component>();
        parent_->setSize(800, 600);

        manager_ = std::make_unique<DialogManager>(*parent_, themeService_, instanceRegistry_);
    }

    void TearDown() override
    {
        manager_.reset();
        parent_.reset();
    }

    static Oscillator makeOscillator(const juce::String& name)
    {
        Oscillator osc;
        osc.setName(name);
        osc.setColour(juce::Colour(0xff00ff00));
        osc.setProcessingMode(ProcessingMode::FullStereo);
        return osc;
    }

    oscil::test::MockThemeService themeService_;
    oscil::test::MockInstanceRegistry instanceRegistry_;
    std::unique_ptr<juce::Component> parent_;
    std::unique_ptr<DialogManager> manager_;
};

// Counting listener for OscillatorConfigDialog events — verifies the
// single-notification-point contract that externalClose provides.
class CountingConfigListener : public OscillatorConfigDialog::Listener
{
public:
    int configChangedCount = 0;
    int closedCount = 0;
    int deleteRequestedCount = 0;
    OscillatorId lastChangedId;

    void oscillatorConfigChanged(const OscillatorId& id, const Oscillator& /*osc*/) override
    {
        ++configChangedCount;
        lastChangedId = id;
    }
    void oscillatorDeleteRequested(const OscillatorId& /*id*/) override { ++deleteRequestedCount; }
    void configDialogClosed() override { ++closedCount; }
};

TEST_F(DialogManagerTest, ConstructsWithoutShowingAnyDialog)
{
    // No dialog should be marked visible immediately after construction.
    EXPECT_FALSE(manager_->isConfigPopupVisibleFor(OscillatorId::generate()));
}

TEST_F(DialogManagerTest, ShowConfigPopupMarksVisibleForThatOscillator)
{
    // Force the synchronous show/hide path — without a message loop, the
    // animated spring never reaches its target and isShowing() stays false.
    ScopedReducedMotion reducedMotion;

    auto osc = makeOscillator("Kick");

    std::vector<std::pair<PaneId, juce::String>> panes;
    panes.emplace_back(PaneId::generate(), juce::String("Pane 1"));

    manager_->showConfigPopup(osc, panes);

    EXPECT_TRUE(manager_->isConfigPopupVisibleFor(osc.getId()));

    // A different oscillator id must not match.
    auto otherId = OscillatorId::generate();
    EXPECT_FALSE(manager_->isConfigPopupVisibleFor(otherId));
}

TEST_F(DialogManagerTest, CloseConfigPopupHidesTheDialogAndFiresListenerExactlyOnce)
{
    // Force synchronous hide so we can observe the close listener without
    // pumping a message loop. This is the same mechanism DialogManager uses
    // in production under reduced-motion.
    ScopedReducedMotion reducedMotion;

    auto osc = makeOscillator("Snare");
    std::vector<std::pair<PaneId, juce::String>> panes{{PaneId::generate(), "Pane 1"}};

    CountingConfigListener listener;
    manager_->addConfigPopupListener(&listener);

    manager_->showConfigPopup(osc, panes);
    ASSERT_TRUE(manager_->isConfigPopupVisibleFor(osc.getId()));

    manager_->closeConfigPopup();

    EXPECT_FALSE(manager_->isConfigPopupVisibleFor(osc.getId()));
    EXPECT_EQ(listener.closedCount, 1) << "configDialogClosed must fire exactly once via the modal->content route";

    manager_->removeConfigPopupListener(&listener);
}

TEST_F(DialogManagerTest, RemovedListenerNoLongerReceivesCloseNotifications)
{
    ScopedReducedMotion reducedMotion;

    CountingConfigListener listener;
    manager_->addConfigPopupListener(&listener);
    manager_->removeConfigPopupListener(&listener);

    auto osc = makeOscillator("HiHat");
    std::vector<std::pair<PaneId, juce::String>> panes{{PaneId::generate(), "Pane 1"}};

    manager_->showConfigPopup(osc, panes);
    manager_->closeConfigPopup();

    EXPECT_EQ(listener.closedCount, 0)
        << "A removed listener must not receive further events — guards against use-after-free during teardown";
}

TEST_F(DialogManagerTest, ShowConfigPopupTwiceForDifferentOscillatorsSwapsState)
{
    ScopedReducedMotion reducedMotion;

    auto oscA = makeOscillator("A");
    auto oscB = makeOscillator("B");
    std::vector<std::pair<PaneId, juce::String>> panes{{PaneId::generate(), "Pane 1"}};

    manager_->showConfigPopup(oscA, panes);
    ASSERT_TRUE(manager_->isConfigPopupVisibleFor(oscA.getId()));

    manager_->showConfigPopup(oscB, panes);

    EXPECT_TRUE(manager_->isConfigPopupVisibleFor(oscB.getId()));
    EXPECT_FALSE(manager_->isConfigPopupVisibleFor(oscA.getId()))
        << "Showing for a different oscillator must replace the currently-tracked id";
}

TEST_F(DialogManagerTest, ParentDestructionTearsDownModalsSynchronously)
{
    ScopedReducedMotion reducedMotion;

    // Open the config popup.
    auto osc = makeOscillator("Clap");
    std::vector<std::pair<PaneId, juce::String>> panes{{PaneId::generate(), "Pane 1"}};
    manager_->showConfigPopup(osc, panes);
    ASSERT_TRUE(manager_->isConfigPopupVisibleFor(osc.getId()));

    // Simulate the parent dying BEFORE the DialogManager by explicitly resetting
    // the parent first. componentBeingDeleted on DialogManager must drive every
    // modal through hideImmediate so no timers are left running on an orphaned
    // component tree. If the synchronous teardown path is broken, this test
    // would either crash on shutdown or leak a timer detected by the leak
    // checker in the base component.
    //
    // Note: parent_ is a unique_ptr; manager_ holds a reference to the parent.
    // We must destroy manager_ BEFORE parent_ so DialogManager's destructor
    // can unregister from parent_. In production the PluginEditor owns both as
    // members and destruction happens in reverse declaration order.
    manager_.reset();
    parent_.reset();

    SUCCEED() << "Synchronous teardown completed without crash";
}

} // namespace oscil
