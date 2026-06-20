#include "DefaultHorizontalNodeGeometry.hpp"

#include "AbstractGraphModel.hpp"
#include "NodeData.hpp"
#include "NodeStyle.hpp"
#include "StyleCollection.hpp"

#include <QPoint>
#include <QRect>
#include <QWidget>

namespace QtNodes {

DefaultHorizontalNodeGeometry::DefaultHorizontalNodeGeometry(AbstractGraphModel &graphModel)
    : AbstractNodeGeometry(graphModel)
    , _portSize(20)
    , _portSpacing(10)
    , _fontMetrics(QFont())
{
    // 端口高度取普通字体高度;粗体度量(标题用)已由基类 AbstractNodeGeometry 持有。
    _portSize = _fontMetrics.height();
}

QRectF DefaultHorizontalNodeGeometry::boundingRect(NodeId const nodeId) const
{
    QSize s = size(nodeId);

    qreal marginSize = 2.0 * _portSpacing;
    QMargins margins(marginSize, marginSize, marginSize, marginSize);

    QRectF r(QPointF(0, 0), s);

    return r.marginsAdded(margins);
}

QSize DefaultHorizontalNodeGeometry::size(NodeId const nodeId) const
{
    return _graphModel.nodeData<QSize>(nodeId, NodeRole::Size);
}

void DefaultHorizontalNodeGeometry::recomputeSize(NodeId const nodeId) const
{
    // —— 横向布局尺寸:头部(常驻)+ 主体(展开时)——
    auto const &nodeStyle = StyleCollection::nodeStyle();
    unsigned int const headerHeight = static_cast<unsigned int>(nodeStyle.HeaderHeight);

    // 头部所需最小宽度 = 三角形区 + 标题宽 + 边距(标题几何由基类 captionRect 提供)。
    QRectF const capRect = captionRect(nodeId);
    unsigned int const headerMinWidth = static_cast<unsigned int>(capRect.width())
                                       + static_cast<unsigned int>(nodeStyle.ArrowSize)
                                       + 3u * static_cast<unsigned int>(nodeStyle.ArrowPadding);

    bool const collapsed = _graphModel.nodeCollapsed(nodeId);

    if (collapsed) {
        // 折叠:仅保留头部。
        unsigned int width = headerMinWidth + 2u * _portSpacing;
        _graphModel.setNodeData(nodeId, NodeRole::Size, QSize(width, headerHeight));
        return;
    }

    // 展开:头部 + 主体。主体高 = 端口区垂直占高 与 内嵌控件高度的较大者。
    unsigned int bodyHeight = maxVerticalPortsExtent(nodeId);

    if (auto w = _graphModel.nodeData<QWidget *>(nodeId, NodeRole::Widget)) {
        bodyHeight = std::max(bodyHeight, static_cast<unsigned int>(w->height()));
    }

    // 标题已进入头部;主体顶部可能有一行 label(nickname)。
    QRectF const lblRect = labelRect(nodeId);
    if (!lblRect.isNull()) {
        bodyHeight += static_cast<unsigned int>(lblRect.height());
        bodyHeight += _portSpacing / 2;
    }

    bodyHeight += _portSpacing; // space above ports
    bodyHeight += _portSpacing; // space below ports

    QVariant var = _graphModel.nodeData(nodeId, NodeRole::ProcessingStatus);

    auto processingStatusValue = var.value<int>();

    if (processingStatusValue != 0)
        bodyHeight += 20;

    unsigned int const height = headerHeight + bodyHeight;

    unsigned int inPortWidth = maxPortsTextAdvance(nodeId, PortType::In);
    unsigned int outPortWidth = maxPortsTextAdvance(nodeId, PortType::Out);

    unsigned int width = inPortWidth + outPortWidth + 4 * _portSpacing;

    if (auto w = _graphModel.nodeData<QWidget *>(nodeId, NodeRole::Widget)) {
        width += w->width();
    }

    unsigned int textWidth = static_cast<unsigned int>(capRect.width());
    if (!lblRect.isNull())
        textWidth = std::max(textWidth, static_cast<unsigned int>(lblRect.width()));

    width = std::max(width, textWidth + 2 * _portSpacing);
    width = std::max(width, headerMinWidth + 2u * _portSpacing); // 至少容得下头部

    _graphModel.setNodeData(nodeId, NodeRole::Size, QSize(width, height));
}

QPointF DefaultHorizontalNodeGeometry::portPosition(NodeId const nodeId,
                                                    PortType const portType,
                                                    PortIndex const portIndex) const
{
    QSize const size = _graphModel.nodeData<QSize>(nodeId, NodeRole::Size);
    auto const &nodeStyle = StyleCollection::nodeStyle();
    double const headerHeight = nodeStyle.HeaderHeight;

    // 折叠:端口不可见,输入收拢到头部左边缘、输出收拢到右边缘。
    if (_graphModel.nodeCollapsed(nodeId)) {
        switch (portType) {
        case PortType::In:
            return QPointF(0.0, headerHeight / 2.0);

        case PortType::Out:
            return QPointF(size.width(), headerHeight / 2.0);

        default:
            return QPointF();
        }
    }

    // 展开:主体从头部下方开始;若顶部有 label,端口区再下移 label 高度。
    unsigned int const step = _portSize + _portSpacing;

    double totalHeight = headerHeight + _portSpacing;
    if (_graphModel.nodeData<bool>(nodeId, NodeRole::LabelVisible)) {
        totalHeight += labelRect(nodeId).height();
        totalHeight += _portSpacing / 2.0;
    }

    totalHeight += step * portIndex;
    totalHeight += step / 2.0;

    switch (portType) {
    case PortType::In:
        return QPointF(0.0, totalHeight);

    case PortType::Out:
        return QPointF(size.width(), totalHeight);

    default:
        return QPointF();
    }
}

QPointF DefaultHorizontalNodeGeometry::portTextPosition(NodeId const nodeId,
                                                        PortType const portType,
                                                        PortIndex const portIndex) const
{
    QPointF p = portPosition(nodeId, portType, portIndex);

    QRectF rect = portTextRect(nodeId, portType, portIndex);

    p.setY(p.y() + rect.height() / 4.0);

    QSize size = _graphModel.nodeData<QSize>(nodeId, NodeRole::Size);

    switch (portType) {
    case PortType::In:
        p.setX(_portSpacing);
        break;

    case PortType::Out:
        p.setX(size.width() - _portSpacing - rect.width());
        break;

    default:
        break;
    }

    return p;
}

QRectF DefaultHorizontalNodeGeometry::labelRect(NodeId const nodeId) const
{
    // 折叠态 label 随主体隐藏。
    if (_graphModel.nodeCollapsed(nodeId))
        return QRect();

    if (!_graphModel.nodeData<bool>(nodeId, NodeRole::LabelVisible))
        return QRect();

    QString nickname = _graphModel.nodeData<QString>(nodeId, NodeRole::Label);

    QRectF nickRect = _boldFontMetrics.boundingRect(nickname);

    nickRect.setWidth(nickRect.width() * 0.5);
    nickRect.setHeight(nickRect.height() * 0.5);

    return nickRect;
}

QPointF DefaultHorizontalNodeGeometry::labelPosition(NodeId const nodeId) const
{
    // 折叠态 label 隐藏。
    if (_graphModel.nodeCollapsed(nodeId))
        return QPointF();

    QRectF const lbl = labelRect(nodeId);
    if (lbl.isNull())
        return QPointF();

    QSize const size = _graphModel.nodeData<QSize>(nodeId, NodeRole::Size);
    double const headerHeight = StyleCollection::nodeStyle().HeaderHeight;

    // label 画在头部下方、主体顶部,水平居中。
    double const y = headerHeight + _portSpacing / 2.0 + lbl.height();
    double const x = 0.5 * (size.width() - lbl.width());

    return QPointF(x, y);
}

QPointF DefaultHorizontalNodeGeometry::widgetPosition(NodeId const nodeId) const
{
    QSize size = _graphModel.nodeData<QSize>(nodeId, NodeRole::Size);

    // 内嵌控件放在头部之下的主体区;若顶部有 label,再下移 label 高度。
    unsigned int topOffset = static_cast<unsigned int>(StyleCollection::nodeStyle().HeaderHeight);
    if (_graphModel.nodeData<bool>(nodeId, NodeRole::LabelVisible))
        topOffset += static_cast<unsigned int>(labelRect(nodeId).height()) + _portSpacing / 2;

    if (auto w = _graphModel.nodeData<QWidget *>(nodeId, NodeRole::Widget)) {
        // If the widget wants to use as much vertical space as possible,
        // place it immediately below the header.
        if (w->sizePolicy().verticalPolicy() & QSizePolicy::ExpandFlag) {
            return QPointF(2.0 * _portSpacing + maxPortsTextAdvance(nodeId, PortType::In),
                           topOffset);
        } else {
            return QPointF(2.0 * _portSpacing + maxPortsTextAdvance(nodeId, PortType::In),
                           (topOffset + size.height() - w->height()) / 2.0);
        }
    }
    return QPointF();
}

QRect DefaultHorizontalNodeGeometry::resizeHandleRect(NodeId const nodeId) const
{
    QSize size = _graphModel.nodeData<QSize>(nodeId, NodeRole::Size);

    unsigned int rectSize = 7;

    return QRect(size.width() - _portSpacing, size.height() - _portSpacing, rectSize, rectSize);
}

QRectF DefaultHorizontalNodeGeometry::portTextRect(NodeId const nodeId,
                                                   PortType const portType,
                                                   PortIndex const portIndex) const
{
    QString s;
    if (_graphModel.portData<bool>(nodeId, portType, portIndex, PortRole::CaptionVisible)) {
        s = _graphModel.portData<QString>(nodeId, portType, portIndex, PortRole::Caption);
    } else {
        auto portData = _graphModel.portData(nodeId, portType, portIndex, PortRole::DataType);

        s = portData.value<NodeDataType>().name;
    }

    return _fontMetrics.boundingRect(s);
}

unsigned int DefaultHorizontalNodeGeometry::maxVerticalPortsExtent(NodeId const nodeId) const
{
    PortCount nInPorts = _graphModel.nodeData<PortCount>(nodeId, NodeRole::InPortCount);

    PortCount nOutPorts = _graphModel.nodeData<PortCount>(nodeId, NodeRole::OutPortCount);

    unsigned int maxNumOfEntries = std::max(nInPorts, nOutPorts);
    unsigned int step = _portSize + _portSpacing;

    return step * maxNumOfEntries;
}

unsigned int DefaultHorizontalNodeGeometry::maxPortsTextAdvance(NodeId const nodeId,
                                                                PortType const portType) const
{
    unsigned int width = 0;

    size_t const n = _graphModel
                         .nodeData(nodeId,
                                   (portType == PortType::Out) ? NodeRole::OutPortCount
                                                               : NodeRole::InPortCount)
                         .toUInt();

    for (PortIndex portIndex = 0ul; portIndex < n; ++portIndex) {
        QString name;

        if (_graphModel.portData<bool>(nodeId, portType, portIndex, PortRole::CaptionVisible)) {
            name = _graphModel.portData<QString>(nodeId, portType, portIndex, PortRole::Caption);
        } else {
            NodeDataType portData = _graphModel.portData<NodeDataType>(nodeId,
                                                                       portType,
                                                                       portIndex,
                                                                       PortRole::DataType);

            name = portData.name;
        }

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        width = std::max(unsigned(_fontMetrics.horizontalAdvance(name)), width);
#else
        width = std::max(unsigned(_fontMetrics.width(name)), width);
#endif
    }

    return width;
}

} // namespace QtNodes
