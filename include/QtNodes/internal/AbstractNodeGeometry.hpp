#pragma once

#include "Definitions.hpp"
#include "Export.hpp"

#include <QFont>
#include <QFontMetrics>
#include <QRectF>
#include <QSize>
#include <QTransform>

namespace QtNodes {

class AbstractGraphModel;

class NODE_EDITOR_PUBLIC AbstractNodeGeometry
{
public:
    AbstractNodeGeometry(AbstractGraphModel &);
    virtual ~AbstractNodeGeometry() {}

    /**
   * The node's size plus some additional margin around it to account for drawing
   * effects (for example shadows) or node's parts outside the size rectangle
   * (for example port points).
   */
    virtual QRectF boundingRect(NodeId const nodeId) const = 0;

    /// A direct rectangle defining the borders of the node's rectangle.
    virtual QSize size(NodeId const nodeId) const = 0;

    /**
   * The function is triggeren when a nuber of ports is changed or when an
   * embedded widget needs an update.
   */
    virtual void recomputeSize(NodeId const nodeId) const = 0;

    /// Port position in node's coordinate system.
    virtual QPointF portPosition(NodeId const nodeId,
                                 PortType const portType,
                                 PortIndex const index) const = 0;

    /// A convenience function using the `portPosition` and a given transformation.
    virtual QPointF portScenePosition(NodeId const nodeId,
                                      PortType const portType,
                                      PortIndex const index,
                                      QTransform const &t) const;

    /// Defines where to draw port label. The point corresponds to a font baseline.
    virtual QPointF portTextPosition(NodeId const nodeId,
                                     PortType const portType,
                                     PortIndex const portIndex) const = 0;

    /**
   * Defines where to start drawing the caption. The point corresponds to a font
   * baseline.
   */
    QPointF captionPosition(NodeId const nodeId) const;

    /// Caption rect (画在头部内,用粗体度量估算)。
    QRectF captionRect(NodeId const nodeId) const;

    /// 头部矩形:节点顶部一条贯穿宽度的横栏,高度=NodeStyle::HeaderHeight。
    QRectF headerRect(NodeId const nodeId) const;

    /// 折叠三角形的"可点击包围盒"(画笔与点击判定共用同一矩形,零漂移)。
    QRectF collapseTriangleRect(NodeId const nodeId) const;

    /**
   * Defines where to start drawing the label. The point corresponds to a font
   * baseline.
   */
    virtual QPointF labelPosition(NodeId const nodeId) const = 0;

    /// Caption rect is needed for estimating the total node size.
    virtual QRectF labelRect(NodeId const nodeId) const = 0;

    /// Position for an embedded widget. Return any value if you don't embed.
    virtual QPointF widgetPosition(NodeId const nodeId) const = 0;

    virtual PortIndex checkPortHit(NodeId const nodeId,
                                   PortType const portType,
                                   QPointF const nodePoint) const;

    virtual QRect resizeHandleRect(NodeId const nodeId) const = 0;

    virtual int getPortSpacing() = 0;

protected:
    AbstractGraphModel &_graphModel;

    // 粗体字体度量,用于估算头部标题尺寸;标 mutable:重算度量不改几何对象的逻辑 const 性。
    mutable QFontMetrics _boldFontMetrics;
};

} // namespace QtNodes
