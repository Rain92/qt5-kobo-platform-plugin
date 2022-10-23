#include "qevdevtouchdata2.h"

#include <QGuiApplication>
#include <QLoggingCategory>

#include "kobofbscreen.h"
#include "qevdevtouchhandler.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcEvdevTouch3, "qt.qpa.input")

QPointF QEvdevTouchScreenData2::transformTouchPoint(const QPointF &p, bool up)
{
    auto canonical_rota = fbink_rota_native_to_canonical(fbink_state->current_rota);
    int32_t dim_swap;
    QPointF canonical_pos;
    QPointF translated_pos;

    if ((canonical_rota & 1U) == 0U)
    {
        // Canonical rotation is even (UR/UD)
        dim_swap = (int32_t)fbink_state->screen_width;
    }
    else
    {
        // Canonical rotation is odd (CW/CCW)
        dim_swap = (int32_t)fbink_state->screen_height;
    }

    // NOTE: The following was borrowed from my experiments with this in InkVT ;).
    // Deal with device-specific rotation quirks...
    // c.f., https://github.com/koreader/koreader/blob/master/frontend/device/kobo/device.lua
    if (fbink_state->device_id == DEVICE_KOBO_TOUCH_AB && up)
    {
        // The Touch A/B does something... weird.
        // The frame that reports a contact lift does the coordinates transform for us...
        // That makes this a NOP for this frame only...
        canonical_pos = p;
    }
    else if (fbink_state->device_id == DEVICE_KOBO_AURA_H2O_2 ||
             fbink_state->device_id == DEVICE_KOBO_LIBRA_2)
    {
        // Aura H2O²r1 & Libra 2
        // !touch_mirrored_x
        canonical_pos = p.transposed();
    }
    else
    {
        // touch_switch_xy && touch_mirrored_x
        canonical_pos.setX(dim_swap - p.y());
        canonical_pos.setY(p.x());
    }

    qCDebug(qLcEvdevTouch3) << "canonical_pos" << canonical_pos;

          // And, finally, handle somewhat standard touch translation given the current rotation
          // c.f., GestureDetector:adjustGesCoordinate @
          // https://github.com/koreader/koreader/blob/master/frontend/device/gesturedetector.lua
    switch (canonical_rota)
    {
        case FB_ROTATE_UR:
            translated_pos = canonical_pos;
            break;
        case FB_ROTATE_CW:
            translated_pos.setX( (int32_t)fbink_state->screen_width - canonical_pos.y());
            translated_pos.setY( canonical_pos.x());
            break;
        case FB_ROTATE_UD:
            translated_pos.setX( (int32_t)fbink_state->screen_width - canonical_pos.x());
            translated_pos.setY( (int32_t)fbink_state->screen_height - canonical_pos.y());
            break;
        case FB_ROTATE_CCW:
            translated_pos.setX( canonical_pos.y());
            translated_pos.setY( (int32_t)fbink_state->screen_height - canonical_pos.x());
            break;
        default:
            translated_pos.setX( -1);
            translated_pos.setY( -1);
            break;
    }

    return translated_pos;
}

QEvdevTouchScreenData2::QEvdevTouchScreenData2(QEvdevTouchScreenHandler *q_ptr, const QStringList &args, KoboFbScreen * koboFbScreen)
    : QEvdevTouchScreenData(q_ptr, args), koboFbScreen(koboFbScreen)
{
    qDebug() << "using experimental touchhandler";
    fbink_state = koboFbScreen->getFBInkState();
}

void QEvdevTouchScreenData2::processInputEvent(input_event *data)
{
    if (data->type == EV_KEY)
    {
        switch (data->code)
        {
            case BTN_TOOL_PEN:
                m_contacts[m_currentSlot].tool = PEN;
                // To detect up/down state on "snow" protocol without weird slot shenanigans...
                // It's out-of-band of MT events, so, it unfortunately means *all* contacts,
                // not a specific slot...
                // (i.e., you won't get an EV_KEY:BTN_TOUCH:0 until *all* contact points have been lifted).
                if (data->value > 0)
                {
                    m_contacts[m_currentSlot].state = Qt::TouchPointPressed;
                }
                else
                {
                    m_contacts[m_currentSlot].state = Qt::TouchPointReleased;
                }
                break;
            case BTN_TOOL_FINGER:
                m_contacts[m_currentSlot].tool = FINGER;
                if (data->value > 0)
                {
                    m_contacts[m_currentSlot].state = Qt::TouchPointPressed;
                }
                else
                {
                    m_contacts[m_currentSlot].state = Qt::TouchPointReleased;
                }
                break;
        }
    }
    else if (data->type == EV_ABS)
    {
        if(data->code == ABS_MT_TOOL_TYPE)
        {
            if (data->value == 0)
            {
                m_contacts[m_currentSlot].tool = FINGER;
            }
            else if (data->value == 1)
            {
                m_contacts[m_currentSlot].tool = PEN;
            }
        }
        if (data->code == ABS_MT_POSITION_X || data->code == ABS_X)
        {
            qCDebug(qLcEvdevTouch3) << "EV_ABS MT_POS_X" << data->value;
            m_currentData.x = qBound(hw_range_x_min, data->value, hw_range_x_max);
            if (m_singleTouch)
            {
                m_contacts[m_currentSlot].x = m_currentData.x;
                qCDebug(qLcEvdevTouch3) << "EV_ABS MT_POS_X Single Touch";
            }
            if (m_typeB)
            {
                qCDebug(qLcEvdevTouch3) << "EV_ABS MT_POS_X Type B";
                m_contacts[m_currentSlot].x = m_currentData.x;
                if (m_contacts[m_currentSlot].state == Qt::TouchPointStationary)
                    m_contacts[m_currentSlot].state = Qt::TouchPointMoved;
            }
        }
        else if (data->code == ABS_MT_POSITION_Y || data->code == ABS_Y)
        {
            m_currentData.y = qBound(hw_range_y_min, data->value, hw_range_y_max);
            qCDebug(qLcEvdevTouch3) << "EV_ABS MT_POS_Y" << data->value;
            if (m_singleTouch)
            {
                m_contacts[m_currentSlot].y = m_currentData.y;
                qCDebug(qLcEvdevTouch3) << "EV_ABS MT_POS_Y Single touch";
            }
            if (m_typeB)
            {
                qCDebug(qLcEvdevTouch3) << "EV_ABS MT_POS_Y Type B";
                m_contacts[m_currentSlot].y = m_currentData.y;
                if (m_contacts[m_currentSlot].state == Qt::TouchPointStationary)
                    m_contacts[m_currentSlot].state = Qt::TouchPointMoved;
            }
        }
        else if (data->code == ABS_MT_TRACKING_ID)
        {
            if (fbink_state->is_sunxi && data->value == -1)
            {
                koboFbScreen->doSunxiPenRefresh();
            }
            else
            {
                m_currentData.trackingId = data->value;

//                // QT apparently can't handle high tracking IDs well?
//                if (m_currentData.trackingId > 2)
//                    m_currentData.trackingId = 1;

                qCDebug(qLcEvdevTouch3) << "EV_ABS TRACKING_ID " << m_currentData.trackingId;
                if (m_typeB)
                {
                    if (m_currentData.trackingId == -1)
                    {
                        m_contacts[m_currentSlot].state = Qt::TouchPointReleased;
                        qCDebug(qLcEvdevTouch3) << "EV_ABS TRACKING_ID = -1 touch point released";
                    }
                    else if (m_currentData.trackingId != m_contacts[m_currentSlot].trackingId)
                    {
                        m_contacts[m_currentSlot].state = Qt::TouchPointPressed;
                        m_contacts[m_currentSlot].trackingId = m_currentData.trackingId;
                        qCDebug(qLcEvdevTouch3) << "EV_ABS TRACKING_ID != -1 touch point pressed";
                    }
                }
            }
        }
        else if (data->code == ABS_MT_TOUCH_MAJOR)
        {
            qCDebug(qLcEvdevTouch3) << "EV_ABS TOUCH_MAJOR";
            m_currentData.maj = data->value;
            if (data->value == 0 && !m_btnTool)
                m_currentData.state = Qt::TouchPointReleased;
            if (m_typeB)
                m_contacts[m_currentSlot].maj = m_currentData.maj;
        }
        else if (data->code == ABS_PRESSURE || data->code == ABS_MT_PRESSURE)
        {
            qCDebug(qLcEvdevTouch3) << "EV_ABS PRESSURE";
            qCDebug(qLcEvdevTouch3, "EV_ABS code 0x%x: pressure %d; bounding to [%d,%d]", data->code,
                        data->value, hw_pressure_min, hw_pressure_max);
            m_currentData.pressure = qBound(hw_pressure_min, data->value, hw_pressure_max);
            if (m_typeB || m_singleTouch)
                m_contacts[m_currentSlot].pressure = m_currentData.pressure;
        }
        else if (data->code == ABS_MT_SLOT)
        {
            m_currentSlot = data->value;
            qCDebug(qLcEvdevTouch3) << "EV_ABS SLOT";
        }
    }
    else if (data->type == EV_KEY && !m_typeB)
    {
        qCDebug(qLcEvdevTouch3) << "EV_KEY";
        if (data->code == BTN_TOUCH && data->value == 0)
        {
            m_contacts[m_currentSlot].state = Qt::TouchPointReleased;
            qCDebug(qLcEvdevTouch3) << "EV_KEY BTN_TOUCH 0 touchpoint released";
        }

        // On some devices (Glo HD) ABS_MT_TRACKING_ID starts at 1
        // => there will be malformed events with ABS_MT_TRACKING_ID 0 => filter those at addTouchPoint().
        if (data->code == BTN_TOUCH && data->value == 1)
        {
            m_contacts[m_currentSlot].state = Qt::TouchPointPressed;
            qCDebug(qLcEvdevTouch3) << "EV_KEY BTN_TOUCH 1 touchpoint pressed";
        }
    }
    else if (data->type == EV_SYN && data->code == SYN_MT_REPORT && m_lastEventType != EV_SYN)
    {
        qCDebug(qLcEvdevTouch3) << "EV_SYN MT_REPORT && lastEvent was not EV_SYN";
        // If there is no tracking id, one will be generated later.
        // Until that use a temporary key.
        int key = m_currentData.trackingId;
        if (key == -1)
            key = m_contacts.count();

        m_contacts.insert(key, m_currentData);
        m_currentData = Contact();
    }
    else if (data->type == EV_SYN && data->code == SYN_REPORT)
    {
        qCDebug(qLcEvdevTouch3) << "EV_SYN SYN_REPORT";

        // Ensure valid IDs even when the driver does not report ABS_MT_TRACKING_ID.
        if (!m_contacts.isEmpty() && m_contacts.constBegin().value().trackingId == -1)
            assignIds();

        std::unique_lock<QMutex> locker;
        if (m_filtered)
            locker = std::unique_lock<QMutex>{m_mutex};

        // update timestamps
        m_lastTimeStamp = m_timeStamp;
        m_timeStamp = data->time.tv_sec + data->time.tv_usec / 1000000.0;

        m_lastTouchPoints = m_touchPoints;
        m_touchPoints.clear();
        Qt::TouchPointStates combinedStates;
        bool hasPressure = false;

        for (auto i = m_contacts.begin(), end = m_contacts.end(); i != end; /*erasing*/)
        {
            auto it = i++;

            Contact &contact(it.value());

            if (!contact.state)
                continue;

            int key = m_typeB ? it.key() : contact.trackingId;
            if (!m_typeB && m_lastContacts.contains(key))
            {
                const Contact &prev(m_lastContacts.value(key));
                if (contact.state == Qt::TouchPointReleased)
                {
                    // Copy over the previous values for released points, just in case.
                    contact.x = prev.x;
                    contact.y = prev.y;
                    contact.maj = prev.maj;
                }
                else
                {
                    contact.state = (prev.x == contact.x && prev.y == contact.y) ? Qt::TouchPointStationary
                                                                                 : Qt::TouchPointMoved;
                }
            }

            // Avoid reporting a contact in released state more than once.
            if (!m_typeB && contact.state == Qt::TouchPointReleased && !m_lastContacts.contains(key))
            {
                m_contacts.erase(it);
                qCDebug(qLcEvdevTouch3) << "Erase contact since touchpoint released";
                continue;
            }

            if (contact.pressure)
                hasPressure = true;

            addTouchPoint(contact, &combinedStates);
        }

        // Now look for contacts that have disappeared since the last sync.
        for (auto it = m_lastContacts.begin(), end = m_lastContacts.end(); it != end; ++it)
        {
            Contact &contact(it.value());
            int key = m_typeB ? it.key() : contact.trackingId;
            if (m_typeB)
            {
                if (contact.trackingId != m_contacts[key].trackingId && contact.state)
                {
                    contact.state = Qt::TouchPointReleased;
                    addTouchPoint(contact, &combinedStates);
                }
            }
            else
            {
                if (!m_contacts.contains(key))
                {
                    contact.state = Qt::TouchPointReleased;
                    addTouchPoint(contact, &combinedStates);
                }
            }
        }

        // Remove contacts that have just been reported as released.
        for (auto i = m_contacts.begin(), end = m_contacts.end(); i != end; /*erasing*/)
        {
            auto it = i++;

            Contact &contact(it.value());

            if (!contact.state)
                continue;

            if (contact.state == Qt::TouchPointReleased)
            {
                if (m_typeB)
                    contact.state = static_cast<Qt::TouchPointState>(0);
                else
                {
                    m_contacts.erase(it);
                    qCDebug(qLcEvdevTouch3) << "Contact Erase since state is released";
                }
            }
            else
            {
                contact.state = Qt::TouchPointStationary;
            }
        }

        m_lastContacts = m_contacts;
        if (!m_typeB && !m_singleTouch)
            m_contacts.clear();

        if (!m_touchPoints.isEmpty() && (hasPressure || combinedStates != Qt::TouchPointStationary))
            reportPoints();
    }

    qCDebug(qLcEvdevTouch3) << "Contact state:" << m_contacts[0].state;

    m_lastEventType = data->type;
}

void QEvdevTouchScreenData2::addTouchPoint(const Contact &contact, Qt::TouchPointStates *combinedStates)
{
//    if (contact.x == -1 && contact.y == -1)
//        return;  // failsafe, for devices with ABS_MT_TRACKING_ID not starting at 0

    QWindowSystemInterface::TouchPoint tp;
    tp.id = contact.trackingId;
    tp.flags = contact.flags;
    tp.state = contact.state;
    *combinedStates |= tp.state;

          // Store the HW coordinates for now, will be updated later.
    tp.area = QRectF(0, 0, contact.maj, contact.maj);
    tp.area.moveCenter(QPoint(contact.x, contact.y));
    tp.pressure = contact.pressure;

    tp.rawPositions.append(QPointF(contact.x, contact.y));

    auto transformedPos = transformTouchPoint(QPointF(contact.x, contact.y), contact.state == Qt::TouchPointReleased);

          // Get a normalized position in range 0..1.
    tp.normalPosition = QPointF(transformedPos.x() / qreal(fbink_state->screen_width),
                                transformedPos.y() / qreal(fbink_state->screen_height));

    qCDebug(qLcEvdevTouch3) << "Adding touch point:" << tp.id << tp.uniqueId;
    qCDebug(qLcEvdevTouch3) << "Touchpoint raw position:" << tp.rawPositions.last();
    qCDebug(qLcEvdevTouch3) << "Touchpoint transformed position:" << transformedPos;
    qCDebug(qLcEvdevTouch3) << "Touchpoint normal transformed position:" << tp.normalPosition;

    m_touchPoints.append(tp);
}

QT_END_NAMESPACE
