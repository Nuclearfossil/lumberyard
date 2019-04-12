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
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Serialization/ObjectStream.h>

namespace AZ
{
    namespace IO
    {
        class FileIOStream;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Bus interface for tools to talk to the LyShine system
//! It is valid to use this bus from resource compilers or the UI Editor
class UiSystemToolsInterface
    : public AZ::EBusTraits
{
public: // types

    class CanvasAssetHandle
    {
    public:
        virtual ~CanvasAssetHandle() {};
    };

public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

    using MutexType = AZStd::recursive_mutex;

    // Public functions

    //! Load a canvas but do not init or activate the entities
    //! The CanvasAssetHandle is an opaque pointer only valid to be passed to the
    //! methods below.
    virtual CanvasAssetHandle* LoadCanvasFromStream(AZ::IO::FileIOStream& stream, const AZ::ObjectStream::FilterDescriptor& filterDesc) = 0;

    //! Save a canvas to a stream
    virtual void SaveCanvasToStream(CanvasAssetHandle* canvas, AZ::IO::FileIOStream& stream) = 0;

    //! Get the slice component for a loaded canvas
    virtual AZ::SliceComponent* GetRootSliceSliceComponent(CanvasAssetHandle* canvas) = 0;

    //! Get the slice entity for a loaded canvas
    virtual AZ::Entity* GetRootSliceEntity(CanvasAssetHandle* canvas) = 0;

    //! Get the canvas entity for a loaded canvas
    virtual AZ::Entity* GetCanvasEntity(CanvasAssetHandle* canvas) = 0;

    //! Replace the slice component with a new one. The old slice component is not deleted.
    //! The client is responsible for that.
    virtual void ReplaceRootSliceSliceComponent(CanvasAssetHandle* canvas, AZ::SliceComponent* newSliceComponent) = 0;

    //! Replace the canvas entity with a new one. The old canvas entity is not deleted.
    //! The client is responsible for that.
    virtual void ReplaceCanvasEntity(CanvasAssetHandle* canvas, AZ::Entity* newCanvasEntity) = 0;
    
    //! Delete the canvas file object and its canvas entity and slice entity.
    virtual void DestroyCanvas(CanvasAssetHandle* canvas) = 0;
};

using UiSystemToolsBus = AZ::EBus<UiSystemToolsInterface>;

