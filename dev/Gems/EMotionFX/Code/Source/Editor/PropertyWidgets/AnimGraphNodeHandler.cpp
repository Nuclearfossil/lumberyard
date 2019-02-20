/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Editor/PropertyWidgets/AnimGraphNodeHandler.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendNodeSelectionWindow.h>
#include <Editor/AnimGraphEditorBus.h>
#include <QHBoxLayout>
#include <QMessageBox>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNodeIdPicker, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNodeIdHandler, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphMotionNodeIdHandler, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphStateIdHandler, AZ::SystemAllocator, 0)

    AnimGraphNodeIdPicker::AnimGraphNodeIdPicker(QWidget* parent)
        : QWidget(parent)
        , m_animGraph(nullptr)
        , m_nodeFilterType(AZ::TypeId::CreateNull())
        , m_showStatesOnly(false)
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);

        m_pickButton = new QPushButton(this);
        connect(m_pickButton, &QPushButton::clicked, this, &AnimGraphNodeIdPicker::OnPickClicked);
        hLayout->addWidget(m_pickButton);

        setLayout(hLayout);
    }


    void AnimGraphNodeIdPicker::SetAnimGraph(AnimGraph* animGraph)
    {
        m_animGraph = animGraph;
        UpdateInterface();
    }


    void AnimGraphNodeIdPicker::UpdateInterface()
    {
        if (!m_nodeId.IsValid())
        {
            m_pickButton->setText("Select node");
        }
        else
        {
            if (m_animGraph)
            {
                AnimGraphNode* node = m_animGraph->RecursiveFindNodeById(m_nodeId);
                if (node)
                {
                    m_pickButton->setText(node->GetName());
                }
            }
        }
    }


    void AnimGraphNodeIdPicker::SetNodeId(AnimGraphNodeId nodeId)
    {
        m_nodeId = nodeId;
        UpdateInterface();
    }


    AnimGraphNodeId AnimGraphNodeIdPicker::GetNodeId() const
    {
        return m_nodeId;
    }


    void AnimGraphNodeIdPicker::SetShowStatesOnly(bool showStatesOnly)
    {
        m_showStatesOnly = showStatesOnly;
    }


    void AnimGraphNodeIdPicker::SetNodeTypeFilter(const AZ::TypeId& nodeFilterType)
    {
        m_nodeFilterType = nodeFilterType;
    }


    void AnimGraphNodeIdPicker::OnPickClicked()
    {
        if (!m_animGraph)
        {
            AZ_Error("EMotionFX", false, "Cannot open anim graph node selection window. No valid anim graph.");
            return;
        }

        // create and show the node picker window
        EMStudio::BlendNodeSelectionWindow dialog(this, true, nullptr, m_nodeFilterType, m_showStatesOnly);
        dialog.Update(m_animGraph->GetID(), nullptr);
        dialog.setModal(true);

        if (dialog.exec() != QDialog::Rejected)
        {
            const AZStd::vector<AnimGraphSelectionItem>& selectedNodes = dialog.GetAnimGraphHierarchyWidget()->GetSelectedItems();
            if (!selectedNodes.empty())
            {
                AnimGraphNode* selectedNode = m_animGraph->RecursiveFindNodeByName(selectedNodes[0].mNodeName.c_str());
                if (selectedNode)
                {
                    m_nodeId = selectedNode->GetId();
                    UpdateInterface();
                    emit SelectionChanged();
                }
            }
        }
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AnimGraphNodeIdHandler::AnimGraphNodeIdHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZ::u64, AnimGraphNodeIdPicker>()
        , m_animGraph(nullptr)
        , m_nodeFilterType(AZ::TypeId::CreateNull())
        , m_showStatesOnly(false)
    {
    }

    AZ::u32 AnimGraphNodeIdHandler::GetHandlerName() const
    {
        return AZ_CRC("AnimGraphNodeId", 0xadadb878);
    }


    QWidget* AnimGraphNodeIdHandler::CreateGUI(QWidget* parent)
    {
        AnimGraphNodeIdPicker* picker = aznew AnimGraphNodeIdPicker(parent);
        picker->SetShowStatesOnly(m_showStatesOnly);
        picker->SetNodeTypeFilter(m_nodeFilterType);

        connect(picker, &AnimGraphNodeIdPicker::SelectionChanged, this, [picker]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
        });

        return picker;
    }


    void AnimGraphNodeIdHandler::ConsumeAttribute(AnimGraphNodeIdPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }

        if (attrib == AZ_CRC("AnimGraph", 0x0d53d4b3))
        {
            attrValue->Read<AnimGraph*>(m_animGraph);
            GUI->SetAnimGraph(m_animGraph);
        }
    }


    void AnimGraphNodeIdHandler::WriteGUIValuesIntoProperty(size_t index, AnimGraphNodeIdPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetNodeId();
    }


    bool AnimGraphNodeIdHandler::ReadValuesIntoGUI(size_t index, AnimGraphNodeIdPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetNodeId(instance);
        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AnimGraphMotionNodeIdHandler::AnimGraphMotionNodeIdHandler()
        : AnimGraphNodeIdHandler()
    {
        m_nodeFilterType = azrtti_typeid<AnimGraphMotionNode>();
    }


    AZ::u32 AnimGraphMotionNodeIdHandler::GetHandlerName() const
    {
        return AZ_CRC("AnimGraphMotionNodeId", 0xe19a0672);
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AnimGraphStateIdHandler::AnimGraphStateIdHandler()
        : AnimGraphNodeIdHandler()
    {
        m_showStatesOnly = true;
    }


    AZ::u32 AnimGraphStateIdHandler::GetHandlerName() const
    {
        return AZ_CRC("AnimGraphStateId", 0x3547298f);
    }
} // namespace EMotionFX

#include <Source/Editor/PropertyWidgets/AnimGraphNodeHandler.moc>