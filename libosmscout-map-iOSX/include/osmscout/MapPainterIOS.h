#ifndef OSMSCOUT_MAP_MAPPAINTERIOS_H
#define OSMSCOUT_MAP_MAPPAINTERIOS_H

/*
 This source is part of the libosmscout-map library
 Copyright (C) 2010  Tim Teulings, Vladimir Vyskocil
 
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
#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#define Font UIFont
#define Image CGImageRef
#else
#import <AppKit/AppKit.h>
#define Font NSFont
#define Image CGImageRef
#define OSMSCOUT_REVERSED_Y_AXIS 1
#endif

#include <osmscout/MapPainter.h>

#define MAP_PAINTER_PLATE_LABEL_MARGIN 10
#define MAP_PAINTER_Y_LABEL_MARGIN 10
#define MAP_PAINTER_DRAW_CONTOUR_LABEL_MARGIN 10

namespace osmscout {
    typedef struct { double x; double y; } Pt;
    typedef struct { double x; double y; double slope ;} XYSlope;
    
    class MapPainterIOS : public MapPainter {
    private:
        CoordBufferImpl<Vertex2D>   *coordBuffer;

        CGContextRef                cg;
        std::vector<Image>          images;         // Cached CGImage for icons
        std::vector<Image>          patternImages;  // Cached CGImage for patterns
        std::map<size_t,Font *>     fonts;          // Cached fonts
        
    public:
        MapPainterIOS();
        virtual ~MapPainterIOS();
        
        bool DrawMap(const StyleConfig& styleConfig,
                     const Projection& projection,
                     const MapParameter& parameter,
                     const MapData& data,
                     CGContextRef paintCG);
    protected:
        bool HasIcon(const StyleConfig& styleConfig,
                     const MapParameter& parameter,
                     IconStyle& style);
        
        bool HasPattern(const MapParameter& parameter,
                        const FillStyle& style);
        
        void GetTextDimension(const MapParameter& parameter,
                              double fontSize,
                              const std::string& text,
                              double& xOff,
                              double& yOff,
                              double& width,
                              double& height);
        
        void DrawContourSymbol(const Projection& projection,
                               const MapParameter& parameter,
                               const Symbol& symbol,
                               double space,
                               size_t transStart, size_t transEnd);
        
        void DrawLabel(const Projection& projection,
                       const MapParameter& parameter,
                       const LabelData& label);
        
        void DrawContourLabel(const Projection& projection,
                              const MapParameter& parameter,
                              const PathTextStyle& style,
                              const std::string& text,
                              size_t transStart, size_t transEnd);
        
        void DrawIcon(const IconStyle* style,
                      double x, double y);
        
        void DrawPrimitivePath(const Projection& projection,
                               const MapParameter& parameter,
                               const DrawPrimitiveRef& primitive,
                               double x, double y,
                               double minX,
                               double minY,
                               double maxX,
                               double maxY);
        
        void DrawSymbol(const Projection& projection,
                        const MapParameter& parameter,
                        const Symbol& symbol,
                        double x, double y);
        
        void DrawPath(const Projection& projection,
                      const MapParameter& parameter,
                      const Color& color,
                      double width,
                      const std::vector<double>& dash,
                      LineStyle::CapStyle startCap,
                      LineStyle::CapStyle endCap,
                      size_t transStart, size_t transEnd);
        
        void DrawArea(const Projection& projection,
                      const MapParameter& parameter,
                      const AreaData& area);
                
        void DrawGround(const Projection& projection,
                        const MapParameter& parameter,
                        const FillStyle& style);
        
        void SetFill(const Projection& projection,
                     const MapParameter& parameter,
                     const FillStyle& fillStyle,
                     CGFloat xOffset=0.0,
                     CGFloat yOffset=0.0);
        
        void SetPen(const LineStyle& style,
                    double lineWidth);

        double textLength(const MapParameter& parameter, double fontSize, std::string text);
        double textHeight(const MapParameter& parameter, double fontSize, std::string text);
        
    private:
        Font *GetFont(const MapParameter& parameter, double fontSize);
        double pathLength(size_t transStart, size_t transEnd);
        XYSlope originAndSlopeAlongPath(CGFloat l, CGFloat nextW, size_t transStart, size_t transEnd,
                                        CGFloat &posX, CGFloat &posY, size_t &i, CGFloat &currentL);
    };
}

#endif
