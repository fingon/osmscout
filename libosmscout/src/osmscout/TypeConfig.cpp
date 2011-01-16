/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/TypeConfig.h>

#include <cassert>

namespace osmscout {

  TagInfo::TagInfo()
   : name("ignore"),
     id(tagIgnore)
  {
  }

  TagInfo::TagInfo(const std::string& name,
                   TagId id)
   : name(name),
     id(id)
  {
    // no code
  }

  TypeInfo::TypeInfo()
   : id(typeIgnore),
     canBeNode(false),
     canBeWay(false),
     canBeArea(false),
     canBeRelation(false),
     canBeRoute(false),
     canBeIndexed(false)
  {
    // no code
  }

  TypeInfo::TypeInfo(TypeId id,
                     TagId tag,
                     const std::string tagValue)
   : id(id),
     tag(tag),
     tagValue(tagValue),
     canBeNode(false),
     canBeWay(false),
     canBeArea(false),
     canBeRelation(false),
     canBeRoute(false),
     canBeIndexed(false)
  {
    // no code
  }

  TypeConfig::TypeConfig()
   : maxTypeId(0)
  {
    AddTagInfo(TagInfo("ignore",tagIgnore));
    AddTagInfo(TagInfo("name",tagName));
    AddTagInfo(TagInfo("ref",tagRef));
    AddTagInfo(TagInfo("oneway",tagOneway));
    AddTagInfo(TagInfo("bridge",tagBridge));
    AddTagInfo(TagInfo("tunnel",tagTunnel));
    AddTagInfo(TagInfo("layer",tagLayer));
    AddTagInfo(TagInfo("building",tagBuilding));
    AddTagInfo(TagInfo("place",tagPlace));
    AddTagInfo(TagInfo("place_name",tagPlaceName));
    AddTagInfo(TagInfo("boundary",tagBoundary));
    AddTagInfo(TagInfo("admin_level",tagAdminLevel));
    AddTagInfo(TagInfo("highway",tagHighway));
    AddTagInfo(TagInfo("restriction",tagRestriction));
    AddTagInfo(TagInfo("type",tagType));
    AddTagInfo(TagInfo("internal",tagInternal));
    AddTagInfo(TagInfo("width",tagWidth));
    AddTagInfo(TagInfo("natural",tagNatural));

    AddTypeInfo(TypeInfo(typeRoute,tagInternal,"route").CanBeWay(true));
  }

  TypeConfig::~TypeConfig()
  {
    // no code
  }

  TypeConfig& TypeConfig::AddTagInfo(const TagInfo& tagInfo)
  {
    tags.push_back(tagInfo);
    stringToTagMap[tagInfo.GetName()]=tagInfo;

    return *this;
  }

  TypeConfig& TypeConfig::AddTypeInfo(const TypeInfo& typeInfo)
  {
    maxTypeId=std::max(maxTypeId,typeInfo.GetId());

    types.push_back(typeInfo);

    tagToTypeMap[typeInfo.GetTag()][typeInfo.GetTagValue()]=typeInfo;
    idToTypeMap[typeInfo.GetId()]=typeInfo;

    return *this;
  }

  TypeId TypeConfig::GetMaxTypeId() const
  {
    return maxTypeId;
  }

  TagId TypeConfig::GetTagId(const char* name) const
  {
    std::map<std::string,TagInfo>::const_iterator iter=stringToTagMap.find(name);

    if (iter!=stringToTagMap.end()) {
      return iter->second.GetId();
    }
    else {
      return tagIgnore;
    }
  }

  const TypeInfo& TypeConfig::GetTypeInfo(TypeId id) const
  {
    std::map<TypeId,TypeInfo>::const_iterator iter=idToTypeMap.find(id);

    assert(iter!=idToTypeMap.end());

    return iter->second;
  }

  bool TypeConfig::GetNodeTypeId(std::vector<Tag> &tags,
                                 std::vector<Tag>::iterator& tag,
                                 TypeId &type) const
  {
    type=typeIgnore;

    for (tag=tags.begin();
         tag!=tags.end();
         ++tag) {
      type=GetNodeTypeId(tag->key,tag->value.c_str());

      if (type!=typeIgnore) {
        return true;
      }
    }

    return false;
  }

  bool TypeConfig::GetWayAreaTypeId(std::vector<Tag>& tags,
                                    std::vector<Tag>::iterator& wayTag,
                                    TypeId &wayType,
                                    std::vector<Tag>::iterator& areaTag,
                                    TypeId &areaType) const
  {
    wayType=typeIgnore;
    areaType=typeIgnore;
    wayTag=tags.end();
    areaTag=tags.end();

    for (std::vector<Tag>::iterator tag=tags.begin();
         tag!=tags.end();
         ++tag) {
      if (wayType==typeIgnore) {
        wayType=GetWayTypeId(tag->key,tag->value.c_str());
        if (wayType!=typeIgnore) {
          wayTag=tag;
        }
      }

      if (areaType==typeIgnore) {
        areaType=GetAreaTypeId(tag->key,tag->value.c_str());
        if (areaType!=typeIgnore) {
          areaTag=tag;
        }
      }

      if (wayType!=typeIgnore && areaType!=typeIgnore) {
        return true;
      }
    }

    return wayType!=typeIgnore || areaType!=typeIgnore;
  }

  bool TypeConfig::GetRelationTypeId(std::vector<Tag> &tags,
                                     std::vector<Tag>::iterator& tag,
                                     TypeId &type) const
  {
    std::string relType;

    for (tag=tags.begin();
         tag!=tags.end();
         ++tag) {
      if (tag->key==tagType) {
        relType=tag->value;
        break;
      }
    }

    type=typeIgnore;

    type=typeIgnore;

    for (tag=tags.begin();
         tag!=tags.end();
         ++tag) {
      type=GetRelationTypeId(tag->key,tag->value.c_str());

      if (type!=typeIgnore) {
        return true;
      }
    }

    return false;
  }

  TypeId TypeConfig::GetNodeTypeId(TagId tagKey, const char* tagValue) const
  {
    std::map<TagId,std::map<std::string,TypeInfo> >::const_iterator iter=tagToTypeMap.find(tagKey);

    if (iter!=tagToTypeMap.end()) {
      std::map<std::string,TypeInfo>::const_iterator iter2=iter->second.find(tagValue);

      if (iter2!=iter->second.end() && iter2->second.CanBeNode()) {
        return iter2->second.GetId();
      }
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetWayTypeId(TagId tagKey, const char* tagValue) const
  {
    std::map<TagId,std::map<std::string,TypeInfo> >::const_iterator iter=tagToTypeMap.find(tagKey);

    if (iter!=tagToTypeMap.end()) {
      std::map<std::string,TypeInfo>::const_iterator iter2=iter->second.find(tagValue);

      if (iter2!=iter->second.end() && iter2->second.CanBeWay()) {
        return iter2->second.GetId();
      }
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetAreaTypeId(TagId tagKey, const char* tagValue) const
  {
    std::map<TagId,std::map<std::string,TypeInfo> >::const_iterator iter=tagToTypeMap.find(tagKey);

    if (iter!=tagToTypeMap.end()) {
      std::map<std::string,TypeInfo>::const_iterator iter2=iter->second.find(tagValue);

      if (iter2!=iter->second.end() && iter2->second.CanBeArea()) {
        return iter2->second.GetId();
      }
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetRelationTypeId(TagId tagKey, const char* tagValue) const
  {
    std::map<TagId,std::map<std::string,TypeInfo> >::const_iterator iter=tagToTypeMap.find(tagKey);

    if (iter!=tagToTypeMap.end()) {
      std::map<std::string,TypeInfo>::const_iterator iter2=iter->second.find(tagValue);

      if (iter2!=iter->second.end() && iter2->second.CanBeRelation()) {
        return iter2->second.GetId();
      }
    }

    return typeIgnore;
  }

  void TypeConfig::GetWaysWithKey(TagId tagKey, std::set<TypeId>& types) const
  {
    std::map<TagId,std::map<std::string,TypeInfo> >::const_iterator iter=tagToTypeMap.find(tagKey);

    if (iter!=tagToTypeMap.end()) {
      for (std::map<std::string,TypeInfo>::const_iterator iter2=iter->second.begin();
           iter2!=iter->second.end();
           ++iter2) {
        if (iter2!=iter->second.end() && iter2->second.CanBeWay()) {
          types.insert(iter2->second.GetId());
        }
      }
    }
  }

  void TypeConfig::GetRoutables(std::set<TypeId>& types) const
  {
    types.clear();

    for (std::list<TypeInfo>::const_iterator type=this->types.begin();
         type!=this->types.end();
         ++type) {
      if (type->CanBeRoute()) {
        types.insert(type->GetId());
      }
    }
  }

  void TypeConfig::GetIndexables(std::set<TypeId>& types) const
  {
    types.clear();

    for (std::list<TypeInfo>::const_iterator type=this->types.begin();
         type!=this->types.end();
         ++type) {
      if (type->CanBeIndexed()) {
        types.insert(type->GetId());
      }
    }
  }
}
