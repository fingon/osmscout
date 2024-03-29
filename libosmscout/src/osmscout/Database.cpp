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

#include <osmscout/Database.h>

#include <algorithm>
#include <iostream>

#if _OPENMP
#include <omp.h>
#endif

#include <osmscout/TypeConfigLoader.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/Geometry.h>

namespace osmscout {

  DatabaseParameter::DatabaseParameter()
  : areaAreaIndexCacheSize(1000),
    areaNodeIndexCacheSize(1000),
    nodeCacheSize(1000),
    wayCacheSize(4000),
    areaCacheSize(4000),
    debugPerformance(false)
  {
    // no code
  }

  void DatabaseParameter::SetAreaAreaIndexCacheSize(unsigned long areaAreaIndexCacheSize)
  {
    this->areaAreaIndexCacheSize=areaAreaIndexCacheSize;
  }

  void DatabaseParameter::SetAreaNodeIndexCacheSize(unsigned long areaNodeIndexCacheSize)
  {
    this->areaNodeIndexCacheSize=areaNodeIndexCacheSize;
  }

  void DatabaseParameter::SetNodeCacheSize(unsigned long nodeCacheSize)
  {
    this->nodeCacheSize=nodeCacheSize;
  }

  void DatabaseParameter::SetWayCacheSize(unsigned long wayCacheSize)
  {
    this->wayCacheSize=wayCacheSize;
  }

  void DatabaseParameter::SetAreaCacheSize(unsigned long areaCacheSize)
  {
    this->areaCacheSize=areaCacheSize;
  }

  void DatabaseParameter::SetDebugPerformance(bool debug)
  {
    debugPerformance=debug;
  }

  unsigned long DatabaseParameter::GetAreaAreaIndexCacheSize() const
  {
    return areaAreaIndexCacheSize;
  }

  unsigned long DatabaseParameter::GetAreaNodeIndexCacheSize() const
  {
    return areaNodeIndexCacheSize;
  }

  unsigned long DatabaseParameter::GetNodeCacheSize() const
  {
    return nodeCacheSize;
  }

  unsigned long DatabaseParameter::GetWayCacheSize() const
  {
    return wayCacheSize;
  }

  unsigned long DatabaseParameter::GetAreaCacheSize() const
  {
    return areaCacheSize;
  }

  bool DatabaseParameter::IsDebugPerformance() const
  {
    return debugPerformance;
  }

  AreaSearchParameter::AreaSearchParameter()
  : maxAreaLevel(4),
    maxNodes(2000),
    maxWays(10000),
    maxAreas(std::numeric_limits<unsigned long>::max()),
    useLowZoomOptimization(true),
    useMultithreading(false)
  {
    // no code
  }

  void AreaSearchParameter::SetMaximumAreaLevel(unsigned long maxAreaLevel)
  {
    this->maxAreaLevel=maxAreaLevel;
  }

  void AreaSearchParameter::SetMaximumNodes(unsigned long maxNodes)
  {
    this->maxNodes=maxNodes;
  }

  void AreaSearchParameter::SetMaximumWays(unsigned long maxWays)
  {
    this->maxWays=maxWays;
  }

  void AreaSearchParameter::SetMaximumAreas(unsigned long maxAreas)
  {
    this->maxAreas=maxAreas;
  }

  void AreaSearchParameter::SetUseLowZoomOptimization(bool useLowZoomOptimization)
  {
    this->useLowZoomOptimization=useLowZoomOptimization;
  }

  void AreaSearchParameter::SetUseMultithreading(bool useMultithreading)
  {
    this->useMultithreading=useMultithreading;
  }

  void AreaSearchParameter::SetBreaker(const BreakerRef& breaker)
  {
    this->breaker=breaker;
  }

  unsigned long AreaSearchParameter::GetMaximumAreaLevel() const
  {
    return maxAreaLevel;
  }

  unsigned long AreaSearchParameter::GetMaximumNodes() const
  {
    return maxNodes;
  }

  unsigned long AreaSearchParameter::GetMaximumWays() const
  {
    return maxWays;
  }

  unsigned long AreaSearchParameter::GetMaximumAreas() const
  {
    return maxAreas;
  }

  bool AreaSearchParameter::GetUseLowZoomOptimization() const
  {
    return useLowZoomOptimization;
  }

  bool AreaSearchParameter::GetUseMultithreading() const
  {
    return useMultithreading;
  }

  bool AreaSearchParameter::IsAborted() const
  {
    if (breaker.Valid()) {
      return breaker->IsAborted();
    }
    else {
      return false;
    }
  }

  Database::Database(const DatabaseParameter& parameter)
   : isOpen(false),
     debugPerformance(parameter.IsDebugPerformance()),
     minLon(0.0),
     minLat(0.0),
     maxLon(0.0),
     maxLat(0.0),
     areaNodeIndex(/*parameter.GetAreaNodeIndexCacheSize()*/),
     areaWayIndex(),
     areaAreaIndex(parameter.GetAreaAreaIndexCacheSize()),
     nodeDataFile("nodes.dat",
                  parameter.GetNodeCacheSize()),
     areaDataFile("areas.dat",
                  parameter.GetAreaCacheSize()),
     wayDataFile("ways.dat",
                  parameter.GetWayCacheSize()),
     typeConfig(NULL)
  {
    // no code
  }

  Database::~Database()
  {
    delete typeConfig;
  }

  bool Database::Open(const std::string& path)
  {
    assert(!path.empty());

    this->path=path;

    typeConfig=new TypeConfig();

    if (!LoadTypeData(path,*typeConfig)) {
      std::cerr << "Cannot load 'types.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    FileScanner scanner;
    std::string file=AppendFileToDir(path,"bounding.dat");

    if (!scanner.Open(file,FileScanner::Normal,true)) {
      std::cerr << "Cannot open 'bounding.dat'" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    uint32_t minLonDat;
    uint32_t minLatDat;
    uint32_t maxLonDat;
    uint32_t maxLatDat;

    scanner.ReadNumber(minLatDat);
    scanner.ReadNumber(minLonDat);
    scanner.ReadNumber(maxLatDat);
    scanner.ReadNumber(maxLonDat);

    if (scanner.HasError() || !scanner.Close()) {
      std::cerr << "Error while reading/closing '" << file << "'" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    minLon=minLonDat/conversionFactor-180.0;
    minLat=minLatDat/conversionFactor-90.0;
    maxLon=maxLonDat/conversionFactor-180.0;
    maxLat=maxLatDat/conversionFactor-90.0;

    if (!nodeDataFile.Open(path,FileScanner::LowMemRandom,true)) {
      std::cerr << "Cannot open 'nodes.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!areaDataFile.Open(path,FileScanner::LowMemRandom,true)) {
      std::cerr << "Cannot open 'areas.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!wayDataFile.Open(path,FileScanner::LowMemRandom,true)) {
      std::cerr << "Cannot open 'ways.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!optimizeAreasLowZoom.Open(path)) {
      std::cerr << "Cannot load area low zoom optimizations!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }


    if (!optimizeWaysLowZoom.Open(path)) {
      std::cerr << "Cannot load ways low zoom optimizations!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!areaAreaIndex.Load(path)) {
      std::cerr << "Cannot load area area index!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!areaNodeIndex.Load(path)) {
      std::cerr << "Cannot load area node index!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!areaWayIndex.Load(path)) {
      std::cerr << "Cannot load area way index!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!waterIndex.Load(path)) {
      std::cerr << "Cannot load water index!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!cityStreetIndex.Load(path)) {
      std::cerr << "Cannot load city street index!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    isOpen=true;

    return true;
  }

  bool Database::IsOpen() const
  {
    return isOpen;
  }


  void Database::Close()
  {
    nodeDataFile.Close();
    wayDataFile.Close();
    areaDataFile.Close();

    optimizeWaysLowZoom.Close();
    optimizeAreasLowZoom.Close();
    areaAreaIndex.Close();
    areaNodeIndex.Close();
    areaWayIndex.Close();

    isOpen=false;
  }

  void Database::FlushCache()
  {
    nodeDataFile.FlushCache();
    areaDataFile.FlushCache();
    wayDataFile.FlushCache();
  }

  std::string Database::GetPath() const
  {
    return path;
  }

  TypeConfig* Database::GetTypeConfig() const
  {
    return typeConfig;
  }

  bool Database::GetBoundingBox(double& minLat,double& minLon,
                                double& maxLat,double& maxLon) const
  {
    if (!IsOpen()) {
      return false;
    }

    minLat=this->minLat;
    minLon=this->minLon;
    maxLat=this->maxLat;
    maxLon=this->maxLon;

    return true;
  }

  bool Database::GetObjectsNodes(const AreaSearchParameter& parameter,
                                 const TypeSet &nodeTypes,
                                 double lonMin, double latMin,
                                 double lonMax, double latMax,
                                 std::string& nodeIndexTime,
                                 std::string& nodesTime,
                                 std::vector<NodeRef>& nodes) const
  {

    if (parameter.IsAborted()) {
      return false;
    }

    nodes.clear();

    if (parameter.IsAborted()) {
      return false;
    }

    std::vector<FileOffset> nodeOffsets;
    StopClock               nodeIndexTimer;

    if (nodeTypes.HasTypes()) {
      if (!areaNodeIndex.GetOffsets(lonMin,latMin,lonMax,latMax,
                                    nodeTypes,
                                    parameter.GetMaximumNodes(),
                                    nodeOffsets)) {
        std::cout << "Error getting nodes from area node index!" << std::endl;
        return false;
      }
    }

    nodeIndexTimer.Stop();
    nodeIndexTime=nodeIndexTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    std::sort(nodeOffsets.begin(),nodeOffsets.end());

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock nodesTimer;

    if (!GetNodesByOffset(nodeOffsets,
                          nodes)) {
      std::cout << "Error reading nodes in area!" << std::endl;
      return false;
    }

    nodesTimer.Stop();
    nodesTime=nodesTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    return true;
  }

  bool Database::GetObjectsAreas(const AreaSearchParameter& parameter,
                                 const TypeSet& areaTypes,
                                 const Magnification& magnification,
                                 double lonMin, double latMin,
                                 double lonMax, double latMax,
                                 std::string& areaOptimizedTime,
                                 std::string& areaIndexTime,
                                 std::string& areasTime,
                                 std::vector<AreaRef>& areas) const
  {
    TypeSet internalAreaTypes(areaTypes);

    if (parameter.IsAborted()) {
      return false;
    }

    std::vector<FileOffset> areaOffsets;
    StopClock               areaOptimizedTimer;

    if (internalAreaTypes.HasTypes()) {
      if (parameter.GetUseLowZoomOptimization() &&
          optimizeAreasLowZoom.HasOptimizations(magnification.GetMagnification())) {
        optimizeAreasLowZoom.GetAreas(lonMin,
                                      latMin,
                                      lonMax,
                                      latMax,
                                      magnification,
                                      parameter.GetMaximumWays(),
                                      internalAreaTypes,
                                      areas);
      }
    }

    areaOptimizedTimer.Stop();
    areaOptimizedTime=areaOptimizedTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    std::vector<FileOffset> offsets;
    StopClock               areaIndexTimer;

    if (internalAreaTypes.HasTypes()) {
      if (!areaAreaIndex.GetOffsets(lonMin,
                                    latMin,
                                    lonMax,
                                    latMax,
                                    magnification.GetLevel()+
                                    parameter.GetMaximumAreaLevel(),
                                    internalAreaTypes,
                                    parameter.GetMaximumAreas(),
                                    offsets)) {
        std::cout << "Error getting areas from area index!" << std::endl;
        return false;
      }
    }

    areaIndexTimer.Stop();
    areaIndexTime=areaIndexTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    std::sort(offsets.begin(),offsets.end());

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock areasTimer;

    if (!offsets.empty()) {
      if (!GetAreasByOffset(offsets,
                            areas)) {
        std::cout << "Error reading areas in area!" << std::endl;
        return false;
      }
    }

    areasTimer.Stop();
    areasTime=areasTimer.ResultString();

    return !parameter.IsAborted();
  }

  bool Database::GetObjectsWays(const AreaSearchParameter& parameter,
                                const std::vector<TypeSet>& wayTypes,
                                const Magnification& magnification,
                                double lonMin, double latMin,
                                double lonMax, double latMax,
                                std::string& wayOptimizedTime,
                                std::string& wayIndexTime,
                                std::string& waysTime,
                                std::vector<WayRef>& ways) const
  {
    std::vector<TypeSet> internalWayTypes(wayTypes);

    if (parameter.IsAborted()) {
      return false;
    }

    std::vector<FileOffset> offsets;
    StopClock               wayOptimizedTimer;

    if (!internalWayTypes.empty()) {
      if (parameter.GetUseLowZoomOptimization() &&
          optimizeWaysLowZoom.HasOptimizations(magnification.GetMagnification())) {
        optimizeWaysLowZoom.GetWays(lonMin,
                                    latMin,
                                    lonMax,
                                    latMax,
                                    magnification,
                                    parameter.GetMaximumWays(),
                                    internalWayTypes,
                                    ways);
      }
    }

    wayOptimizedTimer.Stop();
    wayOptimizedTime=wayOptimizedTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock wayIndexTimer;

    if (!internalWayTypes.empty()) {
      if (!areaWayIndex.GetOffsets(lonMin,
                                   latMin,
                                   lonMax,
                                   latMax,
                                   internalWayTypes,
                                   parameter.GetMaximumWays(),
                                   offsets)) {
        std::cout << "Error getting ways Glations from area way index!" << std::endl;
        return false;
      }
    }

    wayIndexTimer.Stop();
    wayIndexTime=wayIndexTimer.ResultString();

    if (parameter.IsAborted()) {
      return false;
    }

    std::sort(offsets.begin(),offsets.end());

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock waysTimer;

    if (!offsets.empty()) {
      if (!GetWaysByOffset(offsets,
                           ways)) {
        std::cout << "Error reading ways in area!" << std::endl;
        return false;
      }
    }

    waysTimer.Stop();
    waysTime=waysTimer.ResultString();

    return !parameter.IsAborted();
  }

  bool Database::GetObjects(const TypeSet &nodeTypes,
                            const std::vector<TypeSet>& wayTypes,
                            const TypeSet& areaTypes,
                            double lonMin, double latMin,
                            double lonMax, double latMax,
                            const Magnification& magnification,
                            const AreaSearchParameter& parameter,
                            std::vector<NodeRef>& nodes,
                            std::vector<WayRef>& ways,
                            std::vector<AreaRef>& areas) const
  {
    return GetObjects(parameter,
                      magnification,
                      nodeTypes,
                      lonMin,latMin,lonMax,latMax,
                      nodes,
                      wayTypes,
                      lonMin,latMin,lonMax,latMax,
                      ways,
                      areaTypes,
                      lonMin,latMin,lonMax,latMax,
                      areas);
  }

  bool Database::GetObjects(const AreaSearchParameter& parameter,
                            const Magnification& magnification,
                            const TypeSet &nodeTypes,
                            double nodeLonMin, double nodeLatMin,
                            double nodeLonMax, double nodeLatMax,
                            std::vector<NodeRef>& nodes,
                            const std::vector<TypeSet>& wayTypes,
                            double wayLonMin, double wayLatMin,
                            double wayLonMax, double wayLatMax,
                            std::vector<WayRef>& ways,
                            const TypeSet& areaTypes,
                            double areaLonMin, double areaLatMin,
                            double areaLonMax, double areaLatMax,
                            std::vector<AreaRef>& areas) const
  {
    std::string nodeIndexTime;
    std::string nodesTime;

    std::string areaOptimizedTime;
    std::string areaIndexTime;
    std::string areasTime;

    std::string wayOptimizedTime;
    std::string wayIndexTime;
    std::string waysTime;

    if (!IsOpen()) {
      return false;
    }

    nodes.clear();
    ways.clear();
    areas.clear();

    if (parameter.IsAborted()) {
      return false;
    }

    bool nodesSuccess;
    bool waysSuccess;
    bool areasSuccess;

#pragma omp parallel if(parameter.GetUseMultithreading())
#pragma omp sections
    {
#pragma omp section
      nodesSuccess=GetObjectsNodes(parameter,
                                   nodeTypes,
                                   nodeLonMin,
                                   nodeLatMin,
                                   nodeLonMax,
                                   nodeLatMax,
                                   nodeIndexTime,
                                   nodesTime,
                                   nodes);

#pragma omp section
      waysSuccess=GetObjectsWays(parameter,
                                 wayTypes,
                                 magnification,
                                 wayLonMin,
                                 wayLatMin,
                                 wayLonMax,
                                 wayLatMax,
                                 wayOptimizedTime,
                                 wayIndexTime,
                                 waysTime,
                                 ways);

#pragma omp section
      areasSuccess=GetObjectsAreas(parameter,
                                   areaTypes,
                                   magnification,
                                   areaLonMin,
                                   areaLatMin,
                                   areaLonMax,
                                   areaLatMax,
                                   areaOptimizedTime,
                                   areaIndexTime,
                                   areasTime,
                                   areas);
    }

    if (!nodesSuccess ||
        !waysSuccess ||
        !areasSuccess) {
      return false;
    }

    if (debugPerformance) {
      std::cout << "Query: ";
      std::cout << "n " << nodeIndexTime << " ";
      std::cout << "w " << wayIndexTime << " ";
      std::cout << "a " << areaIndexTime << std::endl;

      std::cout << "Load: ";
      std::cout << "n " << nodesTime << " ";
      std::cout << "w " << wayOptimizedTime << "/" << waysTime << " ";
      std::cout << "a " << areaOptimizedTime << "/" << areasTime;
      std::cout << std::endl;
    }

    return true;
  }

  bool Database::GetObjects(double lonMin, double latMin,
                            double lonMax, double latMax,
                            const TypeSet& types,
                            std::vector<NodeRef>& nodes,
                            std::vector<WayRef>& ways,
                            std::vector<AreaRef>& areas) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<TypeSet>    wayTypes;
    std::vector<FileOffset> nodeOffsets;
    std::vector<FileOffset> wayWayOffsets;
    std::vector<FileOffset> wayAreaOffsets;

    nodes.clear();
    ways.clear();
    areas.clear();

    wayTypes.push_back(types);;

    StopClock nodeIndexTimer;

    if (!areaNodeIndex.GetOffsets(lonMin,latMin,lonMax,latMax,
                                  types,
                                  std::numeric_limits<size_t>::max(),
                                  nodeOffsets)) {
      std::cout << "Error getting nodes from area node index!" << std::endl;
      return false;
    }

    nodeIndexTimer.Stop();

    StopClock wayIndexTimer;

    if (!areaWayIndex.GetOffsets(lonMin,
                                 latMin,
                                 lonMax,
                                 latMax,
                                 wayTypes,
                                 std::numeric_limits<size_t>::max(),
                                 wayWayOffsets)) {
      std::cout << "Error getting ways and relations from area way index!" << std::endl;
    }

    wayIndexTimer.Stop();

    StopClock areaAreaIndexTimer;

    if (!areaAreaIndex.GetOffsets(lonMin,
                                  latMin,
                                  lonMax,
                                  latMax,
                                  std::numeric_limits<size_t>::max(),
                                  types,
                                  std::numeric_limits<size_t>::max(),
                                  wayAreaOffsets)) {
      std::cout << "Error getting ways and relations from area index!" << std::endl;
    }

    areaAreaIndexTimer.Stop();

    StopClock sortTimer;

    std::sort(nodeOffsets.begin(),nodeOffsets.end());
    std::sort(wayWayOffsets.begin(),wayWayOffsets.end());
    std::sort(wayAreaOffsets.begin(),wayAreaOffsets.end());

    sortTimer.Stop();

    StopClock nodesTimer;

    if (!GetNodesByOffset(nodeOffsets,
                          nodes)) {
      std::cout << "Error reading nodes in area!" << std::endl;
      return false;
    }

    nodesTimer.Stop();

    StopClock waysTimer;

    if (!GetWaysByOffset(wayWayOffsets,
                         ways)) {
      std::cout << "Error reading ways in area!" << std::endl;
      return false;
    }

    waysTimer.Stop();

    StopClock areasTimer;

    if (!GetAreasByOffset(wayAreaOffsets,
                          areas)) {
      std::cout << "Error reading areas in area!" << std::endl;
      return false;
    }

    areasTimer.Stop();

    if (debugPerformance) {
      std::cout << "I/O: ";
      std::cout << "n " << nodeIndexTimer << " ";
      std::cout << "w " << wayIndexTimer << " ";
      std::cout << "a " << areaAreaIndexTimer;
      std::cout << " - ";
      std::cout << "s "  << sortTimer;
      std::cout << " - ";
      std::cout << "n " << nodesTimer << " ";
      std::cout << "w " << waysTimer << " ";
      std::cout << "a " << areasTimer;
      std::cout << std::endl;
    }

    return true;
  }

  bool Database::GetObjects(const std::set<ObjectFileRef>& objects,
                            OSMSCOUT_HASHMAP<FileOffset,NodeRef>& nodesMap,
                            OSMSCOUT_HASHMAP<FileOffset,AreaRef>& areasMap,
                            OSMSCOUT_HASHMAP<FileOffset,WayRef>& waysMap) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::set<FileOffset> nodeOffsets;
    std::set<FileOffset> areaOffsets;
    std::set<FileOffset> wayOffsets;

    for (std::set<ObjectFileRef>::const_iterator o=objects.begin();
        o!=objects.end();
        ++o) {
      ObjectFileRef object(*o);

      switch (object.GetType()) {
      case osmscout::refNode:
        nodeOffsets.insert(object.GetFileOffset());
        break;
      case osmscout::refArea:
        areaOffsets.insert(object.GetFileOffset());
        break;
      case osmscout::refWay:
        wayOffsets.insert(object.GetFileOffset());
        break;
      default:
        break;
      }
    }

    if (!GetNodesByOffset(nodeOffsets,nodesMap) ||
        !GetAreasByOffset(areaOffsets,areasMap) ||
        !GetWaysByOffset(wayOffsets,waysMap)) {
      std::cerr << "Error while resolving locations" << std::endl;
      return false;
    }

    return true;
  }

  bool Database::GetGroundTiles(double lonMin, double latMin,
                                double lonMax, double latMax,
                                const Magnification& magnification,
                                std::list<GroundTile>& tiles) const
  {
    if (!IsOpen()) {
      return false;
    }

    StopClock timer;

    if (!waterIndex.GetRegions(lonMin,
                               latMin,
                               lonMax,
                               latMax,
                               magnification,
                               tiles)) {
      std::cerr << "Error reading ground tiles in area!" << std::endl;
      return false;
    }

    timer.Stop();

    return true;
  }

  bool Database::GetNodeByOffset(const FileOffset& offset,
                                 NodeRef& node) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<FileOffset> offsets;
    std::vector<NodeRef>    nodes;

    offsets.push_back(offset);

    if (GetNodesByOffset(offsets,nodes)) {
      if (!nodes.empty()) {
        node=*nodes.begin();
        return true;
      }
    }

    return false;
  }

  bool Database::GetNodesByOffset(const std::vector<FileOffset>& offsets,
                                  std::vector<NodeRef>& nodes) const
  {
    if (!IsOpen()) {
      return false;
    }

    return nodeDataFile.GetByOffset(offsets,nodes);
  }

  bool Database::GetNodesByOffset(const std::set<FileOffset>& offsets,
                                  std::vector<NodeRef>& nodes) const
  {
    if (!IsOpen()) {
      return false;
    }

    return nodeDataFile.GetByOffset(offsets,nodes);
  }

  bool Database::GetNodesByOffset(const std::list<FileOffset>& offsets,
                                  std::vector<NodeRef>& nodes) const
  {
    if (!IsOpen()) {
      return false;
    }

    return nodeDataFile.GetByOffset(offsets,nodes);
  }

  bool Database::GetNodesByOffset(const std::set<FileOffset>& offsets,
                                  OSMSCOUT_HASHMAP<FileOffset,NodeRef>& dataMap) const
  {
    if (!IsOpen()) {
      return false;
    }

    return nodeDataFile.GetByOffset(offsets,dataMap);
  }

  bool Database::GetAreaByOffset(const FileOffset& offset,
                                 AreaRef& area) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<FileOffset>  offsets;
    std::vector<AreaRef> areas;

    offsets.push_back(offset);

    if (GetAreasByOffset(offsets,areas)) {
      if (!areas.empty()) {
        area=*areas.begin();
        return true;
      }
    }

    return false;
  }

  bool Database::GetAreasByOffset(const std::vector<FileOffset>& offsets,
                                  std::vector<AreaRef>& areas) const
  {
    if (!IsOpen()) {
      return false;
    }

    return areaDataFile.GetByOffset(offsets,areas);
  }

  bool Database::GetAreasByOffset(const std::set<FileOffset>& offsets,
                                  std::vector<AreaRef>& areas) const
  {
    if (!IsOpen()) {
      return false;
    }

    return areaDataFile.GetByOffset(offsets,areas);
  }

  bool Database::GetAreasByOffset(const std::list<FileOffset>& offsets,
                                  std::vector<AreaRef>& areas) const
  {
    if (!IsOpen()) {
      return false;
    }

    return areaDataFile.GetByOffset(offsets,areas);
  }

  bool Database::GetAreasByOffset(const std::set<FileOffset>& offsets,
                                  OSMSCOUT_HASHMAP<FileOffset,AreaRef>& dataMap) const
  {
    if (!IsOpen()) {
      return false;
    }

    return areaDataFile.GetByOffset(offsets,dataMap);
  }

  bool Database::GetWayByOffset(const FileOffset& offset,
                                WayRef& way) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<FileOffset> offsets;
    std::vector<WayRef>     ways;

    offsets.push_back(offset);

    if (GetWaysByOffset(offsets,ways)) {
      if (!ways.empty()) {
        way=*ways.begin();
        return true;
      }
    }

    return false;
  }

  bool Database::GetWaysByOffset(const std::vector<FileOffset>& offsets,
                                 std::vector<WayRef>& ways) const
  {
    if (!IsOpen()) {
      return false;
    }

    return wayDataFile.GetByOffset(offsets,ways);
  }

  bool Database::GetWaysByOffset(const std::set<FileOffset>& offsets,
                                 std::vector<WayRef>& ways) const
  {
    if (!IsOpen()) {
      return false;
    }

    return wayDataFile.GetByOffset(offsets,ways);
  }

  bool Database::GetWaysByOffset(const std::list<FileOffset>& offsets,
                                 std::vector<WayRef>& ways) const
  {
    if (!IsOpen()) {
      return false;
    }

    return wayDataFile.GetByOffset(offsets,ways);
  }

  bool Database::GetWaysByOffset(const std::set<FileOffset>& offsets,
                                 OSMSCOUT_HASHMAP<FileOffset,WayRef>& dataMap) const
  {
    if (!IsOpen()) {
      return false;
    }

    return wayDataFile.GetByOffset(offsets,dataMap);
  }

  bool Database::VisitAdminRegions(AdminRegionVisitor& visitor) const
  {
    if (!IsOpen()) {
      return false;
    }

    return cityStreetIndex.VisitAdminRegions(visitor);
  }

  bool Database::VisitAdminRegionLocations(const AdminRegion& region,
                                           LocationVisitor& visitor) const
  {
    if (!IsOpen()) {
      return false;
    }

    return cityStreetIndex.VisitAdminRegionLocations(region,
                                                     visitor);
  }

  bool Database::VisitLocationAddresses(const AdminRegion& region,
                                        const Location& location,
                                        AddressVisitor& visitor) const
  {
    if (!IsOpen()) {
      return false;
    }

    return cityStreetIndex.VisitLocationAddresses(region,
                                                  location,
                                                  visitor);
  }

  bool Database::ResolveAdminRegionHierachie(const AdminRegionRef& adminRegion,
                                             std::map<FileOffset,AdminRegionRef >& refs) const
  {
    if (!IsOpen()) {
      return false;
    }

    return cityStreetIndex.ResolveAdminRegionHierachie(adminRegion,
                                                       refs);
  }

  bool Database::HandleAdminRegion(const LocationSearch& search,
                                   const LocationSearch::Entry& searchEntry,
                                   const osmscout::AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                                   LocationSearchResult& result) const
  {
    if (searchEntry.locationPattern.empty()) {
      LocationSearchResult::Entry entry;

      entry.adminRegion=adminRegionResult.adminRegion;

      if (adminRegionResult.isMatch) {
        entry.adminRegionMatchQuality=LocationSearchResult::match;
      }
      else {
        entry.adminRegionMatchQuality=LocationSearchResult::candidate;
      }

      entry.locationMatchQuality=LocationSearchResult::none;
      entry.poiMatchQuality=LocationSearchResult::none;
      entry.addressMatchQuality=LocationSearchResult::none;

      result.results.push_back(entry);

      return true;
    }

    //std::cout << "  Search for location '" << searchEntry.locationPattern << "'" << std::endl;

    osmscout::LocationMatchVisitor visitor(searchEntry.locationPattern,
                                           search.limit>=result.results.size() ? search.limit-result.results.size() : 0);


    if (!VisitAdminRegionLocations(adminRegionResult.adminRegion,
                                   visitor)) {
      return false;
    }

    if (visitor.poiResults.empty() &&
        visitor.locationResults.empty()) {
      // If we search for a location within an area,
      // we do not return the found area as hit, if we
      // did not find the location in it.
      return true;
    }

    for (std::list<osmscout::LocationMatchVisitor::POIResult>::const_iterator poiResult=visitor.poiResults.begin();
        poiResult!=visitor.poiResults.end();
        ++poiResult) {
      if (!HandleAdminRegionPOI(search,
                                adminRegionResult,
                                *poiResult,
                                result)) {
        return false;
      }
    }

    for (std::list<osmscout::LocationMatchVisitor::LocationResult>::const_iterator locationResult=visitor.locationResults.begin();
        locationResult!=visitor.locationResults.end();
        ++locationResult) {
      //std::cout << "  - '" << locationResult->location->name << "'" << std::endl;
      if (!HandleAdminRegionLocation(search,
                                     searchEntry,
                                     adminRegionResult,
                                     *locationResult,
                                     result)) {
        return false;
      }
    }

    return true;
  }

  bool Database::HandleAdminRegionLocation(const LocationSearch& search,
                                           const LocationSearch::Entry& searchEntry,
                                           const osmscout::AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                                           const osmscout::LocationMatchVisitor::LocationResult& locationResult,
                                           LocationSearchResult& result) const
  {
    if (searchEntry.addressPattern.empty()) {
      LocationSearchResult::Entry entry;

      entry.adminRegion=locationResult.adminRegion;
      entry.location=locationResult.location;

      if (adminRegionResult.isMatch) {
        entry.adminRegionMatchQuality=LocationSearchResult::match;
      }
      else {
        entry.adminRegionMatchQuality=LocationSearchResult::candidate;
      }

      if (locationResult.isMatch) {
        entry.locationMatchQuality=LocationSearchResult::match;
      }
      else {
        entry.locationMatchQuality=LocationSearchResult::candidate;
      }

      entry.poiMatchQuality=LocationSearchResult::none;
      entry.addressMatchQuality=LocationSearchResult::none;

      result.results.push_back(entry);

      return true;
    }

    //std::cout << "    Search for address '" << searchEntry.addressPattern << "'" << std::endl;

    osmscout::AddressMatchVisitor visitor(searchEntry.addressPattern,
                                          search.limit>=result.results.size() ? search.limit-result.results.size() : 0);


    if (!VisitLocationAddresses(locationResult.adminRegion,
                                locationResult.location,
                                visitor)) {
      return false;
    }

    if (visitor.results.empty()) {
      LocationSearchResult::Entry entry;

      entry.adminRegion=locationResult.adminRegion;
      entry.location=locationResult.location;

      if (adminRegionResult.isMatch) {
        entry.adminRegionMatchQuality=LocationSearchResult::match;
      }
      else {
        entry.adminRegionMatchQuality=LocationSearchResult::candidate;
      }

      if (locationResult.isMatch) {
        entry.locationMatchQuality=LocationSearchResult::match;
      }
      else {
        entry.locationMatchQuality=LocationSearchResult::candidate;
      }

      entry.poiMatchQuality=LocationSearchResult::none;
      entry.addressMatchQuality=LocationSearchResult::none;

      result.results.push_back(entry);

      return true;
    }

    for (std::list<osmscout::AddressMatchVisitor::AddressResult>::const_iterator addressResult=visitor.results.begin();
        addressResult!=visitor.results.end();
        ++addressResult) {
      //std::cout << "    - '" << addressResult->address->name << "'" << std::endl;
      if (!HandleAdminRegionLocationAddress(search,
                                            adminRegionResult,
                                            locationResult,
                                            *addressResult,
                                            result)) {
        return false;
      }
    }

    return true;
  }

  bool Database::HandleAdminRegionPOI(const LocationSearch& /*search*/,
                                      const osmscout::AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                                      const osmscout::LocationMatchVisitor::POIResult& poiResult,
                                      LocationSearchResult& result) const
  {
    LocationSearchResult::Entry entry;

    entry.adminRegion=adminRegionResult.adminRegion;
    entry.poi=poiResult.poi;

    if (adminRegionResult.isMatch) {
      entry.adminRegionMatchQuality=LocationSearchResult::match;
    }
    else {
      entry.adminRegionMatchQuality=LocationSearchResult::candidate;
    }

    if (poiResult.isMatch) {
      entry.poiMatchQuality=LocationSearchResult::match;
    }
    else {
      entry.poiMatchQuality=LocationSearchResult::candidate;
    }

    entry.locationMatchQuality=LocationSearchResult::none;
    entry.addressMatchQuality=LocationSearchResult::none;

    result.results.push_back(entry);

    return true;
  }

  bool Database::HandleAdminRegionLocationAddress(const LocationSearch& /*search*/,
                                                  const osmscout::AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                                                  const osmscout::LocationMatchVisitor::LocationResult& locationResult,
                                                  const osmscout::AddressMatchVisitor::AddressResult& addressResult,
                                                  LocationSearchResult& result) const
  {
    LocationSearchResult::Entry entry;

    entry.adminRegion=locationResult.adminRegion;
    entry.location=addressResult.location;
    entry.address=addressResult.address;

    if (adminRegionResult.isMatch) {
      entry.adminRegionMatchQuality=LocationSearchResult::match;
    }
    else {
      entry.adminRegionMatchQuality=LocationSearchResult::candidate;
    }

    if (locationResult.isMatch) {
      entry.locationMatchQuality=LocationSearchResult::match;
    }
    else {
      entry.locationMatchQuality=LocationSearchResult::candidate;
    }

    entry.poiMatchQuality=LocationSearchResult::none;

    if (addressResult.isMatch) {
      entry.addressMatchQuality=LocationSearchResult::match;
    }
    else {
      entry.addressMatchQuality=LocationSearchResult::candidate;
    }

    result.results.push_back(entry);

    return true;
  }

  bool Database::SearchForLocations(const LocationSearch& search,
                                    LocationSearchResult& result) const
  {
    result.limitReached=false;
    result.results.clear();

    for (std::list<LocationSearch::Entry>::const_iterator searchEntry=search.searches.begin();
        searchEntry!=search.searches.end();
        ++searchEntry) {
      if (searchEntry->adminRegionPattern.empty()) {
        continue;
      }

      //std::cout << "Search for region '" << searchEntry->adminRegionPattern << "'..." << std::endl;

      osmscout::AdminRegionMatchVisitor adminRegionVisitor(searchEntry->adminRegionPattern,
                                                           search.limit);

      if (!VisitAdminRegions(adminRegionVisitor)) {
        return false;
      }

      if (adminRegionVisitor.limitReached) {
        result.limitReached=true;
      }

      for (std::list<osmscout::AdminRegionMatchVisitor::AdminRegionResult>::const_iterator regionResult=adminRegionVisitor.results.begin();
          regionResult!=adminRegionVisitor.results.end();
          ++regionResult) {
        //std::cout << "- '" << regionResult->adminRegion->name << "', '" << regionResult->adminRegion->aliasName << "'..." << std::endl;

        if (!HandleAdminRegion(search,
                               *searchEntry,
                               *regionResult,
                               result)) {
          return false;
        }
      }
    }

    result.results.sort();
    result.results.unique();

    return true;
  }

  class AdminRegionReverseLookupVisitor : public AdminRegionVisitor
  {
  public:
    struct SearchEntry
    {
      ObjectFileRef         object;
      std::vector<GeoCoord> coords;
    };

  private:
    const Database&                           database;
    std::list<Database::ReverseLookupResult>& results;

    std::list<SearchEntry>                    searchEntries;

  public:
    std::map<FileOffset,AdminRegionRef>       adminRegions;

  public:
    AdminRegionReverseLookupVisitor(const Database& database,
                                    std::list<Database::ReverseLookupResult>& results);

    void AddSearchEntry(const SearchEntry& searchEntry);

    Action Visit(const AdminRegion& region);
  };

  AdminRegionReverseLookupVisitor::AdminRegionReverseLookupVisitor(const Database& database,
                                                                   std::list<Database::ReverseLookupResult>& results)
  : database(database),
    results(results)
  {
    // no code
  }

  void AdminRegionReverseLookupVisitor::AddSearchEntry(const SearchEntry& searchEntry)
  {
    searchEntries.push_back(searchEntry);
  }

  AdminRegionVisitor::Action AdminRegionReverseLookupVisitor::Visit(const AdminRegion& region)
  {
    AreaRef area;
    bool    atLeastOneCandidate=false;

    if (!database.GetAreaByOffset(region.object.GetFileOffset(),
                                  area)) {
      return error;
    }

    for (std::list<SearchEntry>::const_iterator entry=searchEntries.begin();
        entry!=searchEntries.end();
        ++entry) {
      if (region.Match(entry->object)) {
        Database::ReverseLookupResult result;

        result.object=entry->object;
        result.adminRegion=new AdminRegion(region);

        results.push_back(result);
      }

      bool candidate=false;

      for (size_t r=0; r<area->rings.size(); r++) {
        if (area->rings[r].ring!=Area::outerRingId) {
          continue;
        }

        for (std::list<SearchEntry>::const_iterator entry=searchEntries.begin();
            entry!=searchEntries.end();
            ++entry) {
          if (entry->coords.size()==1) {
            if (!IsCoordInArea(entry->coords.front(),
                               area->rings[r].nodes)) {
              continue;
            }
          }
          else {
            if (!IsAreaAtLeastPartlyInArea(entry->coords,
                                           area->rings[r].nodes)) {
              continue;
            }
          }

          // candidate
          candidate=true;
          break;
        }

        if (candidate) {
          break;
        }
      }

      if (candidate) {
        atLeastOneCandidate = true;
        adminRegions.insert(std::make_pair(region.regionOffset,
                                           new AdminRegion(region)));
      }
    }

    if (atLeastOneCandidate) {
      return visitChildren;
    }
    else {
      return skipChildren;
    }
  }

  class LocationReverseLookupVisitor : public LocationVisitor
  {
  public:
    struct Loc
    {
      AdminRegionRef adminRegion;
      LocationRef    location;
    };

  private:
    std::set<ObjectFileRef>                   objects;
    std::list<Database::ReverseLookupResult>& results;

  public:
    std::list<Loc>                            locations;

  public:
    LocationReverseLookupVisitor(std::list<Database::ReverseLookupResult>& results);

    void AddObject(const ObjectFileRef& object);

    bool Visit(const AdminRegion& adminRegion,
               const POI &poi);
    bool Visit(const AdminRegion& adminRegion,
               const Location &location);
  };

  LocationReverseLookupVisitor::LocationReverseLookupVisitor(std::list<Database::ReverseLookupResult>& results)
  : results(results)
  {
    // no code
  }

  void LocationReverseLookupVisitor::AddObject(const ObjectFileRef& object)
  {
    objects.insert(object);
  }

  bool LocationReverseLookupVisitor::Visit(const AdminRegion& adminRegion,
                                           const POI &poi)
  {
    if (objects.find(poi.object)!=objects.end()) {
      Database::ReverseLookupResult result;

      result.object=poi.object;
      result.adminRegion=new AdminRegion(adminRegion);
      result.poi=new POI(poi);

      results.push_back(result);
    }

    return true;
  }

  bool LocationReverseLookupVisitor::Visit(const AdminRegion& adminRegion,
                                           const Location &location)
  {
    Loc l;

    l.adminRegion=new AdminRegion(adminRegion);
    l.location=new Location(location);

    locations.push_back(l);

    for (std::vector<ObjectFileRef>::const_iterator object=location.objects.begin();
        object!=location.objects.end();
        ++object) {
      if (objects.find(*object)!=objects.end()) {
        Database::ReverseLookupResult result;

        result.object=*object;
        result.adminRegion=l.adminRegion;
        result.location=l.location;

        results.push_back(result);
      }
    }

    return true;
  }

  class AddressReverseLookupVisitor : public AddressVisitor
  {
  private:
    std::list<Database::ReverseLookupResult>& results;

    std::set<ObjectFileRef>                   objects;

  public:
    AddressReverseLookupVisitor(std::list<Database::ReverseLookupResult>& results);
    void AddObject(const ObjectFileRef& object);

    bool Visit(const AdminRegion& adminRegion,
               const Location &location,
               const Address& address);
  };

  AddressReverseLookupVisitor::AddressReverseLookupVisitor(std::list<Database::ReverseLookupResult>& results)
  : results(results)
  {
    // no code
  }

  void AddressReverseLookupVisitor::AddObject(const ObjectFileRef& object)
  {
    objects.insert(object);
  }

  bool AddressReverseLookupVisitor::Visit(const AdminRegion& adminRegion,
                                          const Location &location,
                                          const Address& address)
  {
    if (objects.find(address.object)!=objects.end()) {
      Database::ReverseLookupResult result;

      result.object=address.object;
      result.adminRegion=new AdminRegion(adminRegion);
      result.location=new Location(location);
      result.address=new Address(address);

      results.push_back(result);
    }

    return true;
  }

  bool Database::ReverseLookupObjects(const std::list<ObjectFileRef>& objects,
                                      std::list<ReverseLookupResult>& result) const
  {
    result.clear();

    if (!IsOpen()) {
      return false;
    }

    AdminRegionReverseLookupVisitor adminRegionVisitor(*this,
                                                       result);

    for (std::list<ObjectFileRef>::const_iterator object=objects.begin();
        object!=objects.end();
        ++object) {
      std::vector<GeoCoord> coords;

      if (object->GetType()==refNode) {
        NodeRef node;

        if (!GetNodeByOffset(object->GetFileOffset(),
                             node)) {
          return false;
        }

        AdminRegionReverseLookupVisitor::SearchEntry searchEntry;

        searchEntry.object=*object;
        searchEntry.coords.push_back(node->GetCoords());

        adminRegionVisitor.AddSearchEntry(searchEntry);
      }
      else if (object->GetType()==refArea) {
        AreaRef area;

        if (!GetAreaByOffset(object->GetFileOffset(),
                             area)) {
          return false;
        }

        for (size_t r=0; r<area->rings.size(); r++) {
          if (area->rings[r].ring==Area::outerRingId) {
            AdminRegionReverseLookupVisitor::SearchEntry searchEntry;

            searchEntry.object=*object;
            searchEntry.coords=area->rings[r].nodes;

            adminRegionVisitor.AddSearchEntry(searchEntry);
          }
        }
      }
      else if (object->GetType()==refWay) {
        WayRef way;

        if (!GetWayByOffset(object->GetFileOffset(),
                            way)) {
          return false;
        }

        AdminRegionReverseLookupVisitor::SearchEntry searchEntry;

        searchEntry.object=*object;
        searchEntry.coords=way->nodes;

        adminRegionVisitor.AddSearchEntry(searchEntry);
      }
      else {
        return false;
      }
    }

    if (!VisitAdminRegions(adminRegionVisitor)) {
      return false;
    }

    if (adminRegionVisitor.adminRegions.empty()) {
      return true;
    }

    LocationReverseLookupVisitor locationVisitor(result);

    for (std::list<ObjectFileRef>::const_iterator object=objects.begin();
        object!=objects.end();
        ++object) {
      locationVisitor.AddObject(*object);
    }

    for (std::map<FileOffset,AdminRegionRef>::const_iterator regionEntry=adminRegionVisitor.adminRegions.begin();
        regionEntry!=adminRegionVisitor.adminRegions.end();
        ++regionEntry) {
      if (!cityStreetIndex.VisitAdminRegionLocations(*regionEntry->second,
                                                     locationVisitor,
                                                     false)) {
        return false;
      }
    }

    AddressReverseLookupVisitor addressVisitor(result);

    for (std::list<ObjectFileRef>::const_iterator object=objects.begin();
        object!=objects.end();
        ++object) {
      addressVisitor.AddObject(*object);
    }

    for (std::list<LocationReverseLookupVisitor::Loc>::const_iterator location=locationVisitor.locations.begin();
        location!=locationVisitor.locations.end();
        ++location) {

      if (!cityStreetIndex.VisitLocationAddresses(location->adminRegion,
                                                  location->location,
                                                  addressVisitor)) {
        return false;
      }
    }

    return true;
  }

  bool Database::ReverseLookupObject(const ObjectFileRef& object,
                                     std::list<ReverseLookupResult>& result) const
  {
    std::list<ObjectFileRef> objects;

    objects.push_back(object);

    return ReverseLookupObjects(objects,
                                result);
  }

  bool Database::GetClosestRoutableNode(double lat,
                                        double lon,
                                        const osmscout::Vehicle& vehicle,
                                        double radius,
                                        osmscout::ObjectFileRef& object,
                                        size_t& nodeIndex) const
  {
    object.Invalidate();

    double                         topLat;
    double                         botLat;
    double                         leftLon;
    double                         rightLon;
    std::vector<osmscout::NodeRef> nodes;
    std::vector<osmscout::AreaRef> areas;
    std::vector<osmscout::WayRef>  ways;
    double                         minDistance=std::numeric_limits<double>::max();

    osmscout::GetEllipsoidalDistance(lat,
                                     lon,
                                     315.0,
                                     radius,
                                     topLat,
                                     leftLon);

    osmscout::GetEllipsoidalDistance(lat,
                                     lon,
                                     135.0,
                                     radius,
                                     botLat,
                                     rightLon);

    osmscout::TypeSet      routableTypes;

    for (size_t typeId=0; typeId<=typeConfig->GetMaxTypeId(); typeId++) {
      if (typeConfig->GetTypeInfo(typeId).CanRoute(vehicle)) {
        routableTypes.SetType(typeId);
      }
    }

    if (!GetObjects(leftLon,
                    botLat,
                    rightLon,
                    topLat,
                    routableTypes,
                    nodes,
                    ways,
                    areas)) {
      return false;
    }

    // We ignore nodes, we do not assume that they are routable at all

    for (std::vector<osmscout::AreaRef>::const_iterator a=areas.begin();
        a!=areas.end();
        ++a) {
      osmscout::AreaRef area(*a);

      for (size_t i=0; i<area->rings[0].nodes.size(); i++) {
        double distance=sqrt((area->rings[0].nodes[i].GetLat()-lat)*(area->rings[0].nodes[i].GetLat()-lat)+
                             (area->rings[0].nodes[i].GetLon()-lon)*(area->rings[0].nodes[i].GetLon()-lon));

        if (distance<minDistance) {
          minDistance=distance;

          object.Set(area->GetFileOffset(),osmscout::refArea);
          nodeIndex=i;
        }
      }
    }

    for (std::vector<osmscout::WayRef>::const_iterator w=ways.begin();
        w!=ways.end();
        ++w) {
      osmscout::WayRef way(*w);

      for (size_t i=0;  i<way->nodes.size(); i++) {
        double distance=sqrt((way->nodes[i].GetLat()-lat)*(way->nodes[i].GetLat()-lat)+
                             (way->nodes[i].GetLon()-lon)*(way->nodes[i].GetLon()-lon));
        if (distance<minDistance) {
          minDistance=distance;

          object.Set(way->GetFileOffset(),osmscout::refWay);
          nodeIndex=i;
        }
      }
    }

    return true;
  }

  void Database::DumpStatistics()
  {
    nodeDataFile.DumpStatistics();
    areaDataFile.DumpStatistics();
    wayDataFile.DumpStatistics();

    areaAreaIndex.DumpStatistics();
    areaNodeIndex.DumpStatistics();
    areaWayIndex.DumpStatistics();
    cityStreetIndex.DumpStatistics();
    waterIndex.DumpStatistics();
  }
}
