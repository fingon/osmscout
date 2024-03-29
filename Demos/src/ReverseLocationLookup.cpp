/*
  AddressLookup - a demo program for libosmscout
  Copyright (C) 2010  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <cctype>
#include <cstring>

#include <osmscout/Database.h>

#include <osmscout/util/String.h>

int main(int argc, char* argv[])
{
  std::string                        map;
  std::list<osmscout::ObjectFileRef> objects;

  if (argc<4 || argc%2!=0) {
    std::cerr << "AddressLookup <map directory> <ObjectType> <FileOffset>..." << std::endl;
    return 1;
  }

  map=argv[1];

  std::string searchPattern;

  int argIndex=2;
  while (argIndex<argc) {
    osmscout::RefType    objectType=osmscout::refNone;
    osmscout::FileOffset offset=0;

    if (strcmp("Node",argv[argIndex])==0) {
      objectType=osmscout::refNode;
    }
    else if (strcmp("Area",argv[argIndex])==0) {
      objectType=osmscout::refArea;
    }
    else if (strcmp("Way",argv[argIndex])==0) {
      objectType=osmscout::refWay;
    }
    else {
      std::cerr << "Error: ObjectType must be one of 'Node', 'Area' or 'Way'" << std::endl;

      return 1;
    }

    argIndex++;

    if (!osmscout::StringToNumber(argv[argIndex],
                                  offset)) {
      std::cerr << "Error: '" << argv[argIndex] << "' cannot be parsed to a file offset" << std::endl;

      return 1;
    }

    argIndex++;

    objects.push_back(osmscout::ObjectFileRef(offset,
                                              objectType));
  }

  osmscout::DatabaseParameter databaseParameter;
  osmscout::Database          database(databaseParameter);

  if (!database.Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  std::list<osmscout::Database::ReverseLookupResult> result;

  if (database.ReverseLookupObjects(objects,
                                    result)) {
    for (std::list<osmscout::Database::ReverseLookupResult>::const_iterator entry=result.begin();
        entry!=result.end();
        ++entry) {
      std::cout << entry->object.GetTypeName() << " " << entry->object.GetFileOffset() << " matches";

      if (entry->adminRegion.Valid()) {
        std::cout << " region '" << entry->adminRegion->name << "'";
      }

      if (entry->poi.Valid()) {
        std::cout << " poi '" << entry->poi->name << "'";
      }

      if (entry->location.Valid()) {
        std::cout << " location '" << entry->location->name << "'";
      }

      if (entry->address.Valid()) {
        std::cout << " address '" << entry->address->name << "'";
      }

      std::cout << std::endl;
    }
  }
  else {
    std::cerr << "Error while reverse lookup" << std::endl;
  }

  database.Close();

  return 0;
}
