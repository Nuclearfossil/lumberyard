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
#include "precompiled.h"

#include <qmenu.h>

#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>
#include <Editor/Nodes/NodeUtils.h>

#include <ScriptCanvasDeveloperEditor/WrapperMock.h>

namespace ScriptCanvasDeveloper
{
    namespace Nodes
    {
        ////////////////
        // WrapperMock
        ////////////////
        void WrapperMock::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<WrapperMock, Mock>()
                    ->Version(0)
                    ->Field("m_wrappedNodeIds", &WrapperMock::m_wrappedNodeIds)
                    ->Field("m_actionName", &WrapperMock::m_actionName)
                ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<WrapperMock>("WrapperMock", "Node for Mocking Wrapper Node visuals")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(ScriptCanvas::Attributes::Node::NodeType, ScriptCanvasEditor::Nodes::NodeType::WrapperNode)
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &WrapperMock::m_actionName, "Action Name", "The Add Action Button Name")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WrapperMock::OnActionNameChanged)
                        ;
                }
            }
        }

        WrapperMock::WrapperMock()
            : m_actionName("Mock Action")
        {
        }

        void WrapperMock::OnWrapperAction(const QRect&, const QPointF& scenePoint, const QPoint& screenPoint)
        {
            GraphCanvas::GraphId graphId;
            GraphCanvas::SceneMemberRequestBus::EventResult(graphId, GetGraphCanvasNodeId(), &GraphCanvas::SceneMemberRequests::GetScene);

            AZ::EntityId scriptCanvasGraphId;
            ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &ScriptCanvasEditor::GeneralRequests::GetScriptCanvasGraphId, graphId);

            QMenu menu;

            QAction* addMock = menu.addAction("Add Mock Node");
            QAction* addWrapperMock = menu.addAction("Add Wrapper Mock Node");

            QAction* result = menu.exec(screenPoint);

            ScriptCanvasEditor::NodeIdPair nodePair;

            if (result == addMock)
            {
                ScriptCanvasEditor::EditorGraphRequestBus::EventResult(nodePair, scriptCanvasGraphId, &ScriptCanvasEditor::EditorGraphRequests::CreateCustomNode, azrtti_typeid<Mock>(), AZ::Vector2(scenePoint.x(), scenePoint.y()));
            }
            else if (result == addWrapperMock)
            {
                ScriptCanvasEditor::EditorGraphRequestBus::EventResult(nodePair, scriptCanvasGraphId, &ScriptCanvasEditor::EditorGraphRequests::CreateCustomNode, azrtti_typeid<WrapperMock>(), AZ::Vector2(scenePoint.x(), scenePoint.y()));
            }

            if (nodePair.m_scriptCanvasId.IsValid()
                && nodePair.m_graphCanvasId.IsValid())
            {
                m_wrappedNodeIds.push_back(nodePair.m_scriptCanvasId);
                m_graphCanvasMapping[nodePair.m_graphCanvasId] = nodePair.m_scriptCanvasId;

                GraphCanvas::WrappedNodeConfiguration configuration;
                configuration.m_layoutOrder = static_cast<int>(m_wrappedNodeIds.size() - 1);

                GraphCanvas::WrapperNodeRequestBus::Event(GetGraphCanvasNodeId(), &GraphCanvas::WrapperNodeRequests::WrapNode, nodePair.m_graphCanvasId, configuration);
            }
        }

        void WrapperMock::OnGraphCanvasNodeSetup(const GraphCanvas::NodeId& graphCanvasNodeId)
        {
            AZ::EntityId scriptCanvasNodeId = (*MockDescriptorNotificationBus::GetCurrentBusId());
            MockDescriptorNotificationBus::MultiHandler::BusDisconnect(scriptCanvasNodeId);

            GraphCanvas::NodeId nodeId;
            MockDescriptorRequestBus::EventResult(nodeId, scriptCanvasNodeId, &MockDescriptorRequests::GetGraphCanvasNodeId);

            for (int i = 0; i < m_wrappedNodeIds.size(); ++i)
            {
                if (m_wrappedNodeIds[i] == scriptCanvasNodeId)
                {
                    m_graphCanvasMapping[nodeId] = m_wrappedNodeIds[i];

                    GraphCanvas::WrappedNodeConfiguration configuration;
                    configuration.m_layoutOrder = i;

                    GraphCanvas::WrapperNodeRequestBus::Event(GetGraphCanvasNodeId(), &GraphCanvas::WrapperNodeRequests::WrapNode, nodeId, configuration);
                    break;
                }
            }
        }

        void WrapperMock::OnNodeRemoved(const GraphCanvas::NodeId& nodeId)
        {
            auto mapIter = m_graphCanvasMapping.find(nodeId);

            if (mapIter != m_graphCanvasMapping.end())
            {
                AZ::EntityId scriptCanvasNodeId = mapIter->second;

                auto removeIter = AZStd::remove_if(m_wrappedNodeIds.begin(), m_wrappedNodeIds.end(), [scriptCanvasNodeId](const AZ::EntityId& testId) { return scriptCanvasNodeId == testId; });

                if (removeIter != m_wrappedNodeIds.end())
                {
                    m_wrappedNodeIds.erase(removeIter);
                }
            }
        }

        void WrapperMock::OnActionNameChanged()
        {
            GraphCanvas::WrapperNodeRequestBus::Event(GetGraphCanvasNodeId(), &GraphCanvas::WrapperNodeRequests::SetActionString, QString(m_actionName.c_str()));
        }

        void WrapperMock::OnClear()
        {
            GraphCanvas::GraphId graphId;
            GraphCanvas::SceneMemberRequestBus::EventResult(graphId, GetGraphCanvasNodeId(), &GraphCanvas::SceneMemberRequests::GetScene);

            AZStd::unordered_set< AZ::EntityId > deleteIds;

            for (auto& mapPair : m_graphCanvasMapping)
            {
                deleteIds.erase(mapPair.first);
            }

            GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::Delete, deleteIds);
            
            m_wrappedNodeIds.clear();
            m_graphCanvasMapping.clear();
        }

        void WrapperMock::OnNodeDisplayed(const GraphCanvas::NodeId& graphCanvasNodeId)
        {
            ScriptCanvasEditor::ScriptCanvasWrapperNodeDescriptorRequestBus::Handler::BusConnect(graphCanvasNodeId);

            for (unsigned int i = 0; i < m_wrappedNodeIds.size(); ++i)
            {
                GraphCanvas::NodeId nodeId;
                MockDescriptorRequestBus::EventResult(nodeId, m_wrappedNodeIds[i], &MockDescriptorRequests::GetGraphCanvasNodeId);

                if (nodeId.IsValid())
                {
                    m_graphCanvasMapping[nodeId] = m_wrappedNodeIds[i];

                    GraphCanvas::WrappedNodeConfiguration configuration;
                    configuration.m_layoutOrder = i;

                    GraphCanvas::WrapperNodeRequestBus::Event(GetGraphCanvasNodeId(), &GraphCanvas::WrapperNodeRequests::WrapNode, nodeId, configuration);
                }
                else
                {
                    MockDescriptorNotificationBus::MultiHandler::BusConnect(m_wrappedNodeIds[i]);
                }
            }

            OnActionNameChanged();
        }
    }
}
