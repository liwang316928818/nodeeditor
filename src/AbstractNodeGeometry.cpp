#include "AbstractNodeGeometry.hpp"

#include "AbstractGraphModel.hpp"
#include "NodeStyle.hpp"
#include "StyleCollection.hpp"

#include <QMargins>

#include <cmath>

namespace QtNodes {

AbstractNodeGeometry::AbstractNodeGeometry(AbstractGraphModel &graphModel)
    : _graphModel(graphModel)
    , _boldFontMetrics(QFont())
{
    // 头部标题用粗体度量。派生类构造里也会初始化各自的普通字体度量。
    QFont bold;
    bold.setBold(true);
    _boldFontMetrics = QFontMetrics(bold);
}

QRectF AbstractNodeGeometry::headerRect(NodeId const nodeId) const
{
    // 头部:节点顶部贯穿宽度的横栏,固定高度取自 NodeStyle::HeaderHeight。
    QSize const s = size(nodeId);
    int const headerHeight = StyleCollection::nodeStyle().HeaderHeight;
    return QRectF(0, 0, s.width(), headerHeight);
}

QRectF AbstractNodeGeometry::collapseTriangleRect(NodeId const nodeId) const
{
    // 三角形可点击包围盒:位于头部左侧,中心在 (ArrowPadding + ArrowSize/2, HeaderHeight/2)。
    // 画笔与点击判定共用本矩形,保证"画在哪就能点在哪"。
    Q_UNUSED(nodeId); // 位置仅取决于全局头部样式,与具体节点无关;保留参数以与其余几何接口一致。

    auto const &nodeStyle = StyleCollection::nodeStyle();
    int const hs = nodeStyle.HeaderHeight;
    int const as = nodeStyle.ArrowSize;
    int const pad = nodeStyle.ArrowPadding;

    double const cx = pad + as / 2.0;
    double const cy = hs / 2.0;
    // 包围盒略大于三角形本身,便于点击命中。
    double const box = std::max(static_cast<double>(as), hs * 0.6) + 4.0;

    return QRectF(cx - box / 2.0, cy - box / 2.0, box, box);
}

QRectF AbstractNodeGeometry::captionRect(NodeId const nodeId) const
{
    // 标题不可见时返回空矩形(画笔据此跳过绘制)。
    if (!_graphModel.nodeData<bool>(nodeId, NodeRole::CaptionVisible))
        return QRect();

    QString const name = _graphModel.nodeData<QString>(nodeId, NodeRole::Caption);

    return _boldFontMetrics.boundingRect(name);
}

QPointF AbstractNodeGeometry::captionPosition(NodeId const nodeId) const
{
    // 标题画在头部内、三角形右侧。x 起点位于三角形之后,y 为字体基线(头部竖直居中)。
    auto const &nodeStyle = StyleCollection::nodeStyle();
    int const hs = nodeStyle.HeaderHeight;
    int const as = nodeStyle.ArrowSize;
    int const pad = nodeStyle.ArrowPadding;

    double const x = pad * 2.0 + as;
    double const y = hs / 2.0 + captionRect(nodeId).height() / 4.0;

    return QPointF(x, y);
}

QPointF AbstractNodeGeometry::portScenePosition(NodeId const nodeId,
                                                PortType const portType,
                                                PortIndex const index,
                                                QTransform const &t) const
{
    QPointF result = portPosition(nodeId, portType, index);

    return t.map(result);
}

PortIndex AbstractNodeGeometry::checkPortHit(NodeId const nodeId,
                                             PortType const portType,
                                             QPointF const nodePoint) const
{
    // 折叠态下端口不可见,禁止命中(避免向折叠节点新建连接造成歧义)。
    if (_graphModel.nodeCollapsed(nodeId))
        return InvalidPortIndex;

    auto const &nodeStyle = StyleCollection::nodeStyle();

    PortIndex result = InvalidPortIndex;

    if (portType == PortType::None)
        return result;

    double const tolerance = 2.0 * nodeStyle.ConnectionPointDiameter;

    size_t const n = _graphModel.nodeData<unsigned int>(nodeId,
                                                        (portType == PortType::Out)
                                                            ? NodeRole::OutPortCount
                                                            : NodeRole::InPortCount);

    for (unsigned int portIndex = 0; portIndex < n; ++portIndex) {
        auto pp = portPosition(nodeId, portType, portIndex);

        QPointF p = pp - nodePoint;
        auto distance = std::sqrt(QPointF::dotProduct(p, p));

        if (distance < tolerance) {
            result = portIndex;
            break;
        }
    }

    return result;
}

} // namespace QtNodes
