#include "DefaultVerticalNodeGeometry.hpp"

#include "AbstractGraphModel.hpp"
#include "NodeData.hpp"
#include "NodeStyle.hpp"
#include "StyleCollection.hpp"

#include <QPoint>
#include <QRect>
#include <QWidget>

namespace QtNodes {

DefaultVerticalNodeGeometry::DefaultVerticalNodeGeometry(AbstractGraphModel &graphModel)
    : AbstractNodeGeometry(graphModel)
    , _portSize(20)
    , _portSpacing(10)
    , _fontMetrics(QFont())
{
    // 端口高度取普通字体高度;粗体度量(标题用)已由基类 AbstractNodeGeometry 持有。
    _portSize = _fontMetrics.height();
}

QRectF DefaultVerticalNodeGeometry::boundingRect(NodeId const nodeId) const
{
    QSize s = size(nodeId);

    qreal marginSize = 2.0 * _portSpacing;
    QMargins margins(marginSize, marginSize, marginSize, marginSize);

    QRectF r(QPointF(0, 0), s);

    return r.marginsAdded(margins);
}

QSize DefaultVerticalNodeGeometry::size(NodeId const nodeId) const
{
    return _graphModel.nodeData<QSize>(nodeId, NodeRole::Size);
}

void DefaultVerticalNodeGeometry::recomputeSize(NodeId const nodeId) const
{
    // —— 纵向布局尺寸:头部(常驻)+ 主体(展开时)——
    auto const &nodeStyle = StyleCollection::nodeStyle();
    unsigned int const headerHeight = static_cast<unsigned int>(nodeStyle.HeaderHeight);

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

    // 展开:头部 + 主体。
    unsigned int bodyHeight = _portSpacing;

    if (auto w = _graphModel.nodeData<QWidget *>(nodeId, NodeRole::Widget)) {
        bodyHeight = std::max(bodyHeight, static_cast<unsigned int>(w->height()));
    }

    // 标题已进入头部;主体顶部可能有一行 label(nickname)。
    QRectF const lblRect = labelRect(nodeId);
    if (!lblRect.isNull()) {
        bodyHeight += static_cast<unsigned int>(lblRect.height());
        bodyHeight += _portSpacing / 2;
    }

    bodyHeight += _portSpacing;
    bodyHeight += _portSpacing;

    // 顶部和底部分别预留端口标签区的高度。
    bodyHeight += portCaptionsHeight(nodeId, PortType::In);
    bodyHeight += portCaptionsHeight(nodeId, PortType::Out);

    unsigned int const height = headerHeight + bodyHeight;

    PortCount nInPorts = _graphModel.nodeData<PortCount>(nodeId, NodeRole::InPortCount);
    PortCount nOutPorts = _graphModel.nodeData<PortCount>(nodeId, NodeRole::OutPortCount);

    unsigned int inPortWidth = maxPortsTextAdvance(nodeId, PortType::In);
    unsigned int outPortWidth = maxPortsTextAdvance(nodeId, PortType::Out);

    unsigned int totalInPortsWidth = nInPorts > 0
                                         ? inPortWidth * nInPorts + _portSpacing * (nInPorts - 1)
                                         : 0;

    unsigned int totalOutPortsWidth = nOutPorts > 0 ? outPortWidth * nOutPorts
                                                          + _portSpacing * (nOutPorts - 1)
                                                    : 0;

    unsigned int width = std::max(totalInPortsWidth, totalOutPortsWidth);

    if (auto w = _graphModel.nodeData<QWidget *>(nodeId, NodeRole::Widget)) {
        width = std::max(width, static_cast<unsigned int>(w->width()));
    }

    unsigned int textWidth = static_cast<unsigned int>(capRect.width());
    if (!lblRect.isNull())
        textWidth = std::max(textWidth, static_cast<unsigned int>(lblRect.width()));

    width = std::max(width, textWidth);
    width = std::max(width, headerMinWidth); // 至少容得下头部

    width += _portSpacing;
    width += _portSpacing;

    _graphModel.setNodeData(nodeId, NodeRole::Size, QSize(width, height));
}

QPointF DefaultVerticalNodeGeometry::portPosition(NodeId const nodeId,
                                                  PortType const portType,
                                                  PortIndex const portIndex) const
{
    QSize const size = _graphModel.nodeData<QSize>(nodeId, NodeRole::Size);
    auto const &nodeStyle = StyleCollection::nodeStyle();
    double const headerHeight = nodeStyle.HeaderHeight;

    // 折叠:端口不可见,输入收拢到头部顶边中点、输出收拢到底边中点。
    if (_graphModel.nodeCollapsed(nodeId)) {
        switch (portType) {
        case PortType::In:
            return QPointF(size.width() / 2.0, 0.0);

        case PortType::Out:
            return QPointF(size.width() / 2.0, headerHeight);

        default:
            return QPointF();
        }
    }

    // 展开:若顶部有 label,输入端口区下移 label 高度;输出仍在节点底边。
    double topY = headerHeight;
    if (_graphModel.nodeData<bool>(nodeId, NodeRole::LabelVisible)) {
        topY += labelRect(nodeId).height() + _portSpacing / 2.0;
    }

    switch (portType) {
    case PortType::In: {
        unsigned int inPortWidth = maxPortsTextAdvance(nodeId, PortType::In) + _portSpacing;

        PortCount nInPorts = _graphModel.nodeData<PortCount>(nodeId, NodeRole::InPortCount);

        double x = (size.width() - (nInPorts - 1) * inPortWidth) / 2.0 + portIndex * inPortWidth;

        return QPointF(x, topY);
    }

    case PortType::Out: {
        unsigned int outPortWidth = maxPortsTextAdvance(nodeId, PortType::Out) + _portSpacing;
        PortCount nOutPorts = _graphModel.nodeData<PortCount>(nodeId, NodeRole::OutPortCount);

        double x = (size.width() - (nOutPorts - 1) * outPortWidth) / 2.0 + portIndex * outPortWidth;

        return QPointF(x, size.height());
    }

    default:
        return QPointF();
    }
}

QPointF DefaultVerticalNodeGeometry::portTextPosition(NodeId const nodeId,
                                                      PortType const portType,
                                                      PortIndex const portIndex) const
{
    QPointF p = portPosition(nodeId, portType, portIndex);

    QRectF rect = portTextRect(nodeId, portType, portIndex);

    p.setX(p.x() - rect.width() / 2.0);

    QSize size = _graphModel.nodeData<QSize>(nodeId, NodeRole::Size);

    switch (portType) {
    case PortType::In:
        p.setY(5.0 + rect.height());
        break;

    case PortType::Out:
        p.setY(size.height() - 5.0);
        break;

    default:
        break;
    }

    return p;
}

QRectF DefaultVerticalNodeGeometry::labelRect(NodeId const nodeId) const
{
    // 折叠态 label 随主体隐藏。
    if (_graphModel.nodeCollapsed(nodeId))
        return QRect();

    if (!_graphModel.nodeData<bool>(nodeId, NodeRole::LabelVisible))
        return QRect();

    QString nickname = _graphModel.nodeData<QString>(nodeId, NodeRole::Label);

    QRectF rect = _boldFontMetrics.boundingRect(nickname);
    rect.setWidth(rect.width() * 0.5);
    rect.setHeight(rect.height() * 0.5);

    return rect;
}

QPointF DefaultVerticalNodeGeometry::labelPosition(NodeId const nodeId) const
{
    // 折叠态 label 隐藏。
    if (_graphModel.nodeCollapsed(nodeId))
        return QPointF();

    QRectF const rect = labelRect(nodeId);
    if (rect.isNull())
        return QPointF();

    QSize const size = _graphModel.nodeData<QSize>(nodeId, NodeRole::Size);
    double const headerHeight = StyleCollection::nodeStyle().HeaderHeight;

    // label 画在头部下方、主体顶部,水平居中。
    double const y = headerHeight + _portSpacing / 2.0 + rect.height();
    double const x = 0.5 * (size.width() - rect.width());

    return QPointF(x, y);
}

QPointF DefaultVerticalNodeGeometry::widgetPosition(NodeId const nodeId) const
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
            return QPointF(_portSpacing + maxPortsTextAdvance(nodeId, PortType::In), topOffset);
        } else {
            return QPointF(_portSpacing + maxPortsTextAdvance(nodeId, PortType::In),
                           (topOffset + size.height() - w->height()) / 2.0);
        }
    }
    return QPointF();
}

QRect DefaultVerticalNodeGeometry::resizeHandleRect(NodeId const nodeId) const
{
    QSize size = _graphModel.nodeData<QSize>(nodeId, NodeRole::Size);

    unsigned int rectSize = 7;

    return QRect(size.width() - rectSize, size.height() - rectSize, rectSize, rectSize);
}

QRectF DefaultVerticalNodeGeometry::portTextRect(NodeId const nodeId,
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

unsigned int DefaultVerticalNodeGeometry::maxHorizontalPortsExtent(NodeId const nodeId) const
{
    PortCount nInPorts = _graphModel.nodeData<PortCount>(nodeId, NodeRole::InPortCount);

    PortCount nOutPorts = _graphModel.nodeData<PortCount>(nodeId, NodeRole::OutPortCount);

    unsigned int maxNumOfEntries = std::max(nInPorts, nOutPorts);
    unsigned int step = _portSize + _portSpacing;

    return step * maxNumOfEntries;
}

unsigned int DefaultVerticalNodeGeometry::maxPortsTextAdvance(NodeId const nodeId,
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

unsigned int DefaultVerticalNodeGeometry::portCaptionsHeight(NodeId const nodeId,
                                                             PortType const portType) const
{
    unsigned int h = 0;

    switch (portType) {
    case PortType::In: {
        PortCount nInPorts = _graphModel.nodeData<PortCount>(nodeId, NodeRole::InPortCount);
        for (PortIndex i = 0; i < nInPorts; ++i) {
            if (_graphModel.portData<bool>(nodeId, PortType::In, i, PortRole::CaptionVisible)) {
                h += _portSpacing;
                break;
            }
        }
        break;
    }

    case PortType::Out: {
        PortCount nOutPorts = _graphModel.nodeData<PortCount>(nodeId, NodeRole::OutPortCount);
        for (PortIndex i = 0; i < nOutPorts; ++i) {
            if (_graphModel.portData<bool>(nodeId, PortType::Out, i, PortRole::CaptionVisible)) {
                h += _portSpacing;
                break;
            }
        }
        break;
    }

    default:
        break;
    }

    return h;
}

} // namespace QtNodes
