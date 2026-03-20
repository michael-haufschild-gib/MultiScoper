/*
    Oscil - Sidebar Component Tests
*/

#include <gtest/gtest.h>
#include "ui/layout/SidebarComponent.h"
#include "plugin/PluginProcessor.h"
#include "core/InstanceRegistry.h"
#include "core/MemoryBudgetManager.h"
#include "core/ServiceContext.h"
#include "rendering/ShaderRegistry.h"

using namespace oscil;

class SidebarComponentTest : public ::testing::Test
{
protected:
    std::unique_ptr<InstanceRegistry> registry_;
    std::unique_ptr<ThemeManager> themeManager_;
    std::unique_ptr<ShaderRegistry> shaderRegistry_;
    std::unique_ptr<MemoryBudgetManager> memoryBudgetManager_;
    std::unique_ptr<OscilPluginProcessor> processor_;
    std::unique_ptr<SidebarComponent> sidebar_;

    void SetUp() override
    {
        registry_ = std::make_unique<InstanceRegistry>();
        themeManager_ = std::make_unique<ThemeManager>();
        shaderRegistry_ = std::make_unique<ShaderRegistry>();
        memoryBudgetManager_ = std::make_unique<MemoryBudgetManager>();

        processor_ = std::make_unique<OscilPluginProcessor>(*registry_,
                                                            *themeManager_,
                                                            *shaderRegistry_,
                                                            *memoryBudgetManager_);

        ServiceContext context{ *registry_, *themeManager_, *shaderRegistry_ };
        sidebar_ = std::make_unique<SidebarComponent>(context);
    }

    SidebarResizeHandle* findResizeHandle() const
    {
        for (int i = 0; i < sidebar_->getNumChildComponents(); ++i)
        {
            if (auto* handle = dynamic_cast<SidebarResizeHandle*>(sidebar_->getChildComponent(i)))
                return handle;
        }

        return nullptr;
    }
};

TEST_F(SidebarComponentTest, ResizeDragUsesGestureStartWidthForEachUpdate)
{
    sidebar_->setSidebarWidth(300);

    auto* resizeHandle = findResizeHandle();
    ASSERT_NE(resizeHandle, nullptr);
    ASSERT_TRUE(static_cast<bool>(resizeHandle->onResizeStart));
    ASSERT_TRUE(static_cast<bool>(resizeHandle->onResizeDrag));

    resizeHandle->onResizeStart();

    resizeHandle->onResizeDrag(-10);  // Drag left by 10px -> width +10
    EXPECT_EQ(sidebar_->getSidebarWidth(), 310);

    resizeHandle->onResizeDrag(-20);  // Drag left by 20px total -> width +20
    EXPECT_EQ(sidebar_->getSidebarWidth(), 320);
}
