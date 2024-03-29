#ifndef DBTHREAD_H
#define DBTHREAD_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
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

#include <QtGui>
#include <QThread>
#include <QMetaType>
#include <QMutex>

#include <osmscout/Database.h>
#include <osmscout/Router.h>
#include <osmscout/RoutePostprocessor.h>

#include <osmscout/MapPainterQt.h>

#include <osmscout/util/Breaker.h>

#include "Settings.h"

struct RenderMapRequest
{
  double                  lon;
  double                  lat;
  osmscout::Magnification magnification;
  size_t                  width;
  size_t                  height;
};

Q_DECLARE_METATYPE(RenderMapRequest)

struct DatabaseLoadedResponse
{
  double minLat;
  double minLon;
  double maxLat;
  double maxLon;
};

Q_DECLARE_METATYPE(DatabaseLoadedResponse)

class QBreaker : public osmscout::Breaker
{
private:
  mutable QMutex mutex;
  bool           aborted;

public:
  QBreaker();

  bool Break();
  bool IsAborted() const;
  void Reset();
};

class DBThread : public QObject
{
  Q_OBJECT

signals:
  void InitialisationFinished(const DatabaseLoadedResponse& response);
  void HandleMapRenderingResult();
  void Redraw();
  void stylesheetFilenameChanged();

public slots:
  void TriggerMapRendering();
  void Initialize();
  void Finalize();
  void ReloadStyle(const QString &suffix="");

private:
  SettingsRef                  settings;
  mutable QMutex               mutex;
  osmscout::DatabaseParameter  databaseParameter;
  osmscout::Database           database;
  osmscout::StyleConfig        *styleConfig;
  osmscout::MapData            data;
  osmscout::MapPainterQt       painter;
  osmscout::RouterParameter    routerParameter;
  osmscout::RouterRef          router;
  osmscout::RoutePostprocessor routePostprocessor;
  QString                      iconDirectory;
  QString                      m_stylesheetFilename;

  QImage                       *currentImage;
  double                       currentLat;
  double                       currentLon;
  osmscout::Magnification      currentMagnification;

  QImage                       *finishedImage;
  double                       finishedLat;
  double                       finishedLon;
  osmscout::Magnification      finishedMagnification;

  RenderMapRequest             currentRenderRequest;
  bool                         doRender;
  QBreaker*                    renderBreaker;
  osmscout::BreakerRef         renderBreakerRef;

private:
  DBThread(const SettingsRef& settings);

  void FreeMaps();
  bool AssureRouter(osmscout::Vehicle vehicle);
public:
  QString stylesheetFilename();

  void UpdateRenderRequest(const RenderMapRequest& request);

  bool RenderMap(QPainter& painter,
                 const RenderMapRequest& request);

  osmscout::TypeConfig* GetTypeConfig() const;

  bool GetNodeByOffset(osmscout::FileOffset offset,
                       osmscout::NodeRef& node) const;
  bool GetAreaByOffset(osmscout::FileOffset offset,
                       osmscout::AreaRef& relation) const;
  bool GetWayByOffset(osmscout::FileOffset offset,
                      osmscout::WayRef& way) const;

  bool ResolveAdminRegionHierachie(const osmscout::AdminRegionRef& adminRegion,
                                   std::map<osmscout::FileOffset,osmscout::AdminRegionRef >& refs) const;

  bool SearchForLocations(const osmscout::LocationSearch& search,
                          osmscout::LocationSearchResult& result) const;

  bool CalculateRoute(osmscout::Vehicle vehicle,
                      const osmscout::RoutingProfile& routingProfile,
                      const osmscout::ObjectFileRef& startObject,
                      size_t startNodeIndex,
                      const osmscout::ObjectFileRef targetObject,
                      size_t targetNodeIndex,
                      osmscout::RouteData& route);

  bool TransformRouteDataToRouteDescription(osmscout::Vehicle vehicle,
                                            const osmscout::RoutingProfile& routingProfile,
                                            const osmscout::RouteData& data,
                                            osmscout::RouteDescription& description,
                                            const std::string& start,
                                            const std::string& target);
  bool TransformRouteDataToWay(osmscout::Vehicle vehicle,
                               const osmscout::RouteData& data,
                               osmscout::Way& way);

  bool GetClosestRoutableNode(const osmscout::ObjectFileRef& refObject,
                              const osmscout::Vehicle& vehicle,
                              double radius,
                              osmscout::ObjectFileRef& object,
                              size_t& nodeIndex);

  void ClearRoute();
  void AddRoute(const osmscout::Way& way);

  static bool InitializeInstance(const SettingsRef& settings);
  static DBThread* GetInstance();
  static void FreeInstance();
};

#endif
