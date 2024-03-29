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

#include <osmscout/Location.h>

#include <osmscout/util/String.h>

namespace osmscout {

  bool AdminRegion::Match(const ObjectFileRef& object) const
  {
    if (this->object==object) {
      return true;
    }

    if (aliasObject==object) {
      return true;
    }

    if (object.GetType()==refNode) {
      for (std::vector<AdminRegion::RegionAlias>::const_iterator alias=aliases.begin();
          alias!=aliases.end();
          ++alias) {
        if (alias->objectOffset==object.GetFileOffset()) {
          return true;
        }
      }
    }

    return false;
  }

  AdminRegionVisitor::~AdminRegionVisitor()
  {
    // no code
  }

  AdminRegionMatchVisitor::AdminRegionMatchVisitor(const std::string& pattern,
                                                   size_t limit)
  : pattern(pattern),
    limit(limit),
    limitReached(false)
  {
    // no code
  }

  void AdminRegionMatchVisitor::Match(const std::string& name,
                                      bool& match,
                                      bool& candidate) const
  {
    std::string::size_type matchPosition;

    matchPosition=name.find(pattern);

    match=matchPosition==0 && name.length()==pattern.length();
    candidate=matchPosition!=std::string::npos;
  }

  AdminRegionVisitor::Action AdminRegionMatchVisitor::Visit(const AdminRegion& region)
  {
    bool match;
    bool candidate;

    Match(region.name,
          match,
          candidate);

    if (match || candidate) {
      AdminRegionResult result;

      result.adminRegion=new AdminRegion(region);
      result.isMatch=match;

      results.push_back(result);
    }

    for (size_t i=0; i<region.aliases.size(); i++) {
      Match(region.aliases[i].name,
            match,
            candidate);

      if (match || candidate) {
        AdminRegionResult result;

        result.adminRegion=new AdminRegion(region);
        result.isMatch=match;

        result.adminRegion->aliasName=region.aliases[i].name;
        result.adminRegion->aliasObject.Set(region.aliases[i].objectOffset,refNode);

        results.push_back(result);

        limitReached=results.size()>=limit;
      }
    }

    if (limitReached) {
      return stop;
    }
    else {
      return visitChildren;
    }
  }

  LocationVisitor::~LocationVisitor()
  {
    // no code
  }

  LocationMatchVisitor::LocationMatchVisitor(const std::string& pattern,
                                             size_t limit)
  : pattern(pattern),
    limit(limit),
    limitReached(false)
  {
    // no code
  }

  void LocationMatchVisitor::Match(const std::string& name,
                                   bool& match,
                                   bool& candidate) const
  {
    std::string::size_type matchPosition;

    matchPosition=name.find(pattern);

    match=matchPosition==0 && name.length()==pattern.length();
    candidate=matchPosition!=std::string::npos;
  }

  bool LocationMatchVisitor::Visit(const AdminRegion& adminRegion,
                                   const POI &poi)
  {
    bool match;
    bool candidate;

    Match(poi.name,
          match,
          candidate);

    if (match || candidate) {
      POIResult result;

      result.adminRegion=new AdminRegion(adminRegion);
      result.poi=new POI(poi);
      result.isMatch=match;

      poiResults.push_back(result);

      limitReached=poiResults.size()+locationResults.size()>=limit;
    }

    return !limitReached;
  }

  bool LocationMatchVisitor::Visit(const AdminRegion& adminRegion,
                                   const Location &loc)
  {
    bool match;
    bool candidate;

    Match(loc.name,
          match,
          candidate);

    if (match || candidate) {
      LocationResult result;

      result.adminRegion=new AdminRegion(adminRegion);
      result.location=new Location(loc);
      result.isMatch=match;

      locationResults.push_back(result);

      limitReached=poiResults.size()+locationResults.size()>=limit;
    }

    return !limitReached;
  }

  AddressVisitor::~AddressVisitor()
  {
    // no code
  }

  AddressListVisitor::AddressListVisitor(size_t limit)
  : limit(limit),
    limitReached(false)
  {
    // no code
  }

  bool AddressListVisitor::Visit(const Location& location,
                                 const Address& address)
  {
    AddressResult result;

    result.location=new Location(location);
    result.address=new Address(address);

    results.push_back(result);

    limitReached=results.size()>=limit;

    return !limitReached;
  }

  AddressMatchVisitor::AddressMatchVisitor(const std::string& pattern,
                                           size_t limit)
  : pattern(pattern),
    limit(limit),
    limitReached(false)
  {
    // no code
  }

  void AddressMatchVisitor::Match(const std::string& name,
                                  bool& match,
                                  bool& candidate) const
  {
    std::string::size_type matchPosition;

    matchPosition=name.find(pattern);

    match=matchPosition==0 && name.length()==pattern.length();
    candidate=matchPosition!=std::string::npos;
  }

  bool AddressMatchVisitor::Visit(const AdminRegion& adminRegion,
                                  const Location& location,
                                  const Address& address)
  {
    bool match;
    bool candidate;

    Match(address.name,
          match,
          candidate);

    if (match || candidate) {
      AddressResult result;

      result.location=new Location(location);
      result.address=new Address(address);
      result.isMatch=match;

      results.push_back(result);

      limitReached=results.size()>=limit;
    }

    return !limitReached;
  }

  void LocationSearch::InitializeSearchEntries(const std::string& searchPattern)
  {
    std::list<std::string> tokens;

    if (searchPattern.empty()) {
      return;
    }

    TokenizeString(searchPattern,
                   tokens);

    if (tokens.empty()) {
      return;
    }

    SimplifyTokenList(tokens);

    if (tokens.size()>=3) {
      std::list<std::list<std::string> > slices;

      GroupStringListToStrings(tokens.begin(),
                               tokens.size(),
                               3,
                               slices);

      for (std::list< std::list<std::string> >::const_iterator slice=slices.begin();
          slice!=slices.end();
          ++slice) {
        std::list<std::string>::const_iterator text1;
        std::list<std::string>::const_iterator text2;
        std::list<std::string>::const_iterator text3;

        text1=slice->begin();
        text2=text1;
        text2++;
        text3=text2;
        text3++;

        osmscout::LocationSearch::Entry search;

        search.locationPattern=*text1;
        search.addressPattern=*text2;
        search.adminRegionPattern=*text3;

        searches.push_back(search);

        search.locationPattern=*text2;
        search.addressPattern=*text3;
        search.adminRegionPattern=*text1;

        searches.push_back(search);
      }
    }

    if (tokens.size()>=2) {
      std::list<std::list<std::string> > slices;

      GroupStringListToStrings(tokens.begin(),
                               tokens.size(),
                               2,
                               slices);

      for (std::list< std::list<std::string> >::const_iterator slice=slices.begin();
          slice!=slices.end();
          ++slice) {
        std::list<std::string>::const_iterator text1;
        std::list<std::string>::const_iterator text2;

        text1=slice->begin();
        text2=text1;
        text2++;

        osmscout::LocationSearch::Entry search;

        search.locationPattern=*text1;
        search.adminRegionPattern=*text2;

        searches.push_back(search);

        search.locationPattern=*text2;
        search.adminRegionPattern=*text1;

        searches.push_back(search);
      }
    }

    if (tokens.size()>=1) {
      std::list<std::list<std::string> > slices;

      GroupStringListToStrings(tokens.begin(),
                               tokens.size(),
                               1,
                               slices);

      for (std::list< std::list<std::string> >::const_iterator slice=slices.begin();
          slice!=slices.end();
          ++slice) {
        std::list<std::string>::const_iterator text1=slice->begin();

        osmscout::LocationSearch::Entry search;

        search.adminRegionPattern=*text1;

        searches.push_back(search);
      }
    }
  }

  bool LocationSearchResult::Entry::operator<(const Entry& other) const
  {
    if (adminRegionMatchQuality!=other.adminRegionMatchQuality) {
      return adminRegionMatchQuality<other.adminRegionMatchQuality;
    }
    else if (locationMatchQuality!=other.locationMatchQuality) {
      return locationMatchQuality<other.locationMatchQuality;
    }
    else if (addressMatchQuality!=other.addressMatchQuality) {
      return addressMatchQuality<other.addressMatchQuality;
    }
    else if (poiMatchQuality!=other.poiMatchQuality) {
      return poiMatchQuality<other.poiMatchQuality;
    }
    else if (adminRegion.Valid() && other.adminRegion.Valid() &&
        adminRegion->name!=other.adminRegion->name) {
      return adminRegion->name<other.adminRegion->name;
    }
    else if (location.Valid() && other.location.Valid() &&
        location->name!=other.location->name) {
      return location->name<other.location->name;
    }
    else if (address.Valid() && other.address.Valid() &&
        address->name!=other.address->name) {
      return address->name<other.address->name;
    }
    else if (poi.Valid() && other.poi.Valid() &&
        poi->name!=other.poi->name) {
      return poi->name<other.poi->name;
    }

    return this<&other;
  }

  bool LocationSearchResult::Entry::operator==(const Entry& other) const
  {
    if ((adminRegion.Valid() && !other.adminRegion.Valid()) ||
        (!adminRegion.Valid() && other.adminRegion.Valid())) {
      return false;
    }

    if (adminRegion.Valid() && other.adminRegion.Valid()) {
      if (adminRegion->aliasObject!=other.adminRegion->aliasObject) {
        return false;
      }

      if (adminRegion->object!=other.adminRegion->object) {
        return false;
      }
    }

    if ((poi.Valid() && !other.poi.Valid()) ||
        (!poi.Valid() && other.poi.Valid())) {
      return false;
    }

    if (poi.Valid() && other.poi.Valid()) {
      if (poi->object!=other.poi->object) {
        return false;
      }
    }

    if ((location.Valid() && !other.location.Valid()) ||
        (!location.Valid() && other.location.Valid())) {
      return false;
    }

    if (location.Valid() && other.location.Valid()) {
      if (location->locationOffset!=other.location->locationOffset) {
        return false;
      }
    }

    if ((address.Valid() && !other.address.Valid()) ||
        (!address.Valid() && other.address.Valid())) {
      return false;
    }

    if (address.Valid() && other.address.Valid()) {
      if (address->addressOffset!=other.address->addressOffset) {
        return false;
      }
    }

    return true;
  }
}

