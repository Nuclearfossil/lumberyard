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
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/containers/vector.h>

#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/SourceThumbnail.h>
#include <AzToolsFramework/Thumbnails/SourceControlThumbnail.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>

#include <QVariant>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        SourceAssetBrowserEntry::~SourceAssetBrowserEntry()
        {
            EntryCache::GetInstance()->m_fileIdMap.erase(m_fileId);

            if (m_sourceId != -1) 
            {
                EntryCache::GetInstance()->m_sourceUuidMap.erase(m_sourceUuid);
                EntryCache::GetInstance()->m_sourceIdMap.erase(m_sourceId);
            }
        }

        QVariant SourceAssetBrowserEntry::data(int column) const
        {
            switch (static_cast<Column>(column))
            {
            case Column::SourceID:
            {
                return QVariant(m_sourceId);
            }
            case Column::ScanFolderID:
            {
                return QVariant(m_scanFolderId);
            }
            default:
            {
                return AssetBrowserEntry::data(column);
            }
            }
        }

        void SourceAssetBrowserEntry::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<SourceAssetBrowserEntry, AssetBrowserEntry>()
                    ->Version(2)
                    ->Field("m_sourceId", &SourceAssetBrowserEntry::m_sourceId)
                    ->Field("m_scanFolderId", &SourceAssetBrowserEntry::m_scanFolderId)
                    ->Field("m_sourceUuid", &SourceAssetBrowserEntry::m_sourceUuid)
                    ->Field("m_extension", &SourceAssetBrowserEntry::m_extension);
            }
        }

        AssetBrowserEntry::AssetEntryType SourceAssetBrowserEntry::GetEntryType() const
        {
            return AssetEntryType::Source;
        }

        const AZStd::string& SourceAssetBrowserEntry::GetExtension() const
        {
            return m_extension;
        }

        AZ::s64 SourceAssetBrowserEntry::GetFileID() const
        {
            return m_fileId;
        }

        const AZ::Uuid& SourceAssetBrowserEntry::GetSourceUuid() const
        {
            return m_sourceUuid;
        }

        AZ::s64 SourceAssetBrowserEntry::GetSourceID() const
        {
            return m_sourceId;
        }

        AZ::s64 SourceAssetBrowserEntry::GetScanFolderID() const
        {
            return m_scanFolderId;
        }

        AZ::Data::AssetType SourceAssetBrowserEntry::GetPrimaryAssetType() const
        {
            AZStd::vector<const ProductAssetBrowserEntry*> products;
            GetChildren<ProductAssetBrowserEntry>(products);
            for (const ProductAssetBrowserEntry* product : products)
            {
                AZ::Data::AssetType productType = product->GetAssetType();
                if (productType != AZ::Data::s_invalidAssetType)
                {
                    return productType;
                }
            }

            return AZ::Data::s_invalidAssetType;
        }

        bool SourceAssetBrowserEntry::HasProductType(const AZ::Data::AssetType& assetType) const
        {
            AZStd::vector<const ProductAssetBrowserEntry*> products;
            GetChildren<ProductAssetBrowserEntry>(products);
            for (const ProductAssetBrowserEntry* product : products)
            {
                AZ::Data::AssetType productType = product->GetAssetType();
                if (productType == assetType)
                {
                    return true;
                }
            }
            return false;
        }

        const SourceAssetBrowserEntry* SourceAssetBrowserEntry::GetSourceByUuid(const AZ::Uuid& sourceUuid)
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                return cache->m_sourceUuidMap[sourceUuid];
            }
            return nullptr;
        }

        void SourceAssetBrowserEntry::UpdateChildPaths(AssetBrowserEntry* child) const
        {
            child->m_fullPath = m_fullPath;
            AssetBrowserEntry::UpdateChildPaths(child);
        }

        void SourceAssetBrowserEntry::PathsUpdated()
        {
            AssetBrowserEntry::PathsUpdated();
            UpdateSourceControlThumbnail();
        }

        void SourceAssetBrowserEntry::UpdateSourceControlThumbnail()
        {
            if (m_sourceControlThumbnailKey)
            {
                disconnect(m_sourceControlThumbnailKey.data(), &ThumbnailKey::ThumbnailUpdatedSignal, this, &AssetBrowserEntry::ThumbnailUpdated);
            }
            m_sourceControlThumbnailKey = MAKE_TKEY(SourceControlThumbnailKey, m_fullPath.c_str());
            connect(m_sourceControlThumbnailKey.data(), &ThumbnailKey::ThumbnailUpdatedSignal, this, &AssetBrowserEntry::ThumbnailUpdated);
        }

        SharedThumbnailKey SourceAssetBrowserEntry::CreateThumbnailKey()
        {
            return MAKE_TKEY(SourceThumbnailKey, m_fullPath.c_str());
        }

        SharedThumbnailKey SourceAssetBrowserEntry::GetSourceControlThumbnailKey() const
        {
            return m_sourceControlThumbnailKey;
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include <AssetBrowser/Entries/SourceAssetBrowserEntry.moc>