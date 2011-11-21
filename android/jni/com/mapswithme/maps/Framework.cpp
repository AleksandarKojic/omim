/*
 * Framework.cpp
 *
 *  Created on: Oct 13, 2011
 *      Author: siarheirachytski
 */

#include "Framework.hpp"
#include "VideoTimer.hpp"

#include "../core/jni_helper.hpp"
#include "../core/render_context.hpp"

#include "../../../../../indexer/drawing_rules.hpp"

#include "../../../../../map/partial_render_policy.hpp"
#include "../../../../../map/framework.hpp"

#include "../../../../../std/shared_ptr.hpp"
#include "../../../../../std/bind.hpp"


#include "../../../../../yg/framebuffer.hpp"
#include "../../../../../yg/internal/opengl.hpp"

#include "../../../../../platform/platform.hpp"
#include "../../../../../base/logging.hpp"
#include "../../../../../base/math.hpp"

android::Framework * g_framework = 0;

namespace android
{
  void Framework::CallRepaint()
  {
    //LOG(LINFO, ("Calling Repaint"));
  }

  Framework::Framework(JavaVM * jvm)
   : m_work(),
     m_eventType(NVMultiTouchEventType(0)),
     m_hasFirst(false),
     m_hasSecond(false),
     m_mask(0)
  {
    ASSERT(g_framework == 0, ());
    g_framework = this;

    m_videoTimer = new VideoTimer(jvm, bind(&Framework::CallRepaint, this));

   // @TODO refactor storage
    m_work.Storage().ReInitCountries(false);
  }

  Framework::~Framework()
  {
    delete m_videoTimer;
  }

  namespace
  {
    struct make_all_invalid
    {
      make_all_invalid()
      {}

      void operator() (int, int, int, drule::BaseRule * p)
      {
        p->MakeEmptyID();
        p->MakeEmptyID2();
      }
    };
  }

  void Framework::DeleteRenderPolicy()
  {
    LOG(LINFO, ("clearing current render policy."));
    m_work.SetRenderPolicy(0);
    LOG(LINFO, ("cleaning all cached ruleID values."));
    drule::rules().ForEachRule(make_all_invalid());
  }

  void Framework::InitRenderPolicy()
  {
    LOG(LDEBUG, ("AF::InitRenderer 1"));

    DrawerYG::Params params;
    params.m_frameBuffer = make_shared_ptr(new yg::gl::FrameBuffer(true));

    yg::ResourceManager::Params rmParams;
    rmParams.m_videoMemoryLimit = 15 * 1024 * 1024;
    rmParams.m_rtFormat = yg::Rt8Bpp;

    m_work.SetRenderPolicy(new PartialRenderPolicy(m_videoTimer,
                                                   params,
                                                   rmParams,
                                                   make_shared_ptr(new android::RenderContext())));

    m_work.SetUpdatesEnabled(true);

    LOG(LDEBUG, ("AF::InitRenderer 3"));
  }

  storage::Storage & Framework::Storage()
  {
    return m_work.Storage();
  }

  void Framework::Resize(int w, int h)
  {
    m_work.OnSize(w, h);
  }

  void Framework::DrawFrame()
  {
    if (m_work.NeedRedraw())
    {
      m_work.SetNeedRedraw(false);

      shared_ptr<PaintEvent> paintEvent(new PaintEvent(m_work.GetRenderPolicy()->GetDrawer().get()));

      m_work.BeginPaint(paintEvent);
      m_work.DoPaint(paintEvent);

      NVEventSwapBuffersEGL();

      m_work.EndPaint(paintEvent);
    }
  }

  void Framework::Move(int mode, double x, double y)
  {
    DragEvent e(x, y);
    switch (mode)
    {
    case 0: m_work.StartDrag(e); break;
    case 1: m_work.DoDrag(e); break;
    case 2: m_work.StopDrag(e); break;
    }
  }

  void Framework::Zoom(int mode, double x1, double y1, double x2, double y2)
  {
    ScaleEvent e(x1, y1, x2, y2);
    switch (mode)
    {
    case 0: m_work.StartScale(e); break;
    case 1: m_work.DoScale(e); break;
    case 2: m_work.StopScale(e); break;
    }
  }

  void Framework::Touch(int action, int mask, double x1, double y1, double x2, double y2)
  {
    NVMultiTouchEventType eventType = (NVMultiTouchEventType)action;

    if (m_mask != mask)
    {
      if (m_mask == 0x0)
      {
        if (mask == 0x1)
          m_work.StartDrag(DragEvent(x1, y1));

        if (mask == 0x2)
          m_work.StartDrag(DragEvent(x2, y2));

        if (mask == 0x3)
          m_work.StartScale(ScaleEvent(x1, y1, x2, y2));
      }

      if (m_mask == 0x1)
      {
        m_work.StopDrag(DragEvent(x1, y1));

        if (mask == 0x0)
        {
          if ((eventType != NV_MULTITOUCH_UP) && (eventType != NV_MULTITOUCH_CANCEL))
            LOG(LINFO, ("should be NV_MULTITOUCH_UP or NV_MULTITOUCH_CANCEL"));
        }

        if (m_mask == 0x2)
          m_work.StartDrag(DragEvent(x2, y2));

        if (mask == 0x3)
          m_work.StartScale(ScaleEvent(x1, y1, x2, y2));
      }

      if (m_mask == 0x2)
      {
        m_work.StopDrag(DragEvent(x2, y2));

        if (mask == 0x0)
        {
          if ((eventType != NV_MULTITOUCH_UP) && (eventType != NV_MULTITOUCH_CANCEL))
            LOG(LINFO, ("should be NV_MULTITOUCH_UP or NV_MULTITOUCH_CANCEL"));
        }

        if (mask == 0x1)
          m_work.StartDrag(DragEvent(x1, y1));

        if (mask == 0x3)
          m_work.StartScale(ScaleEvent(x1, y1, x2, y2));
      }

      if (m_mask == 0x3)
      {
        m_work.StopScale(ScaleEvent(m_x1, m_y1, m_x2, m_y2));

        if ((eventType == NV_MULTITOUCH_MOVE))
        {
          if (mask == 0x1)
            m_work.StartDrag(DragEvent(x1, y1));

          if (mask == 0x2)
            m_work.StartDrag(DragEvent(x2, y2));
        }
        else
          mask = 0;
      }
    }
    else
    {
      if (eventType == NV_MULTITOUCH_MOVE)
      {
        if (m_mask == 0x1)
          m_work.DoDrag(DragEvent(x1, y1));
        if (m_mask == 0x2)
          m_work.DoDrag(DragEvent(x2, y2));
        if (m_mask == 0x3)
          m_work.DoScale(ScaleEvent(x1, y1, x2, y2));
      }

      if ((eventType == NV_MULTITOUCH_CANCEL) || (eventType == NV_MULTITOUCH_UP))
      {
        if (m_mask == 0x1)
          m_work.StopDrag(DragEvent(x1, y1));
        if (m_mask == 0x2)
          m_work.StopDrag(DragEvent(x2, y2));
        if (m_mask == 0x3)
          m_work.StopScale(ScaleEvent(m_x1, m_y1, m_x2, m_y2));
        mask = 0;
      }
    }

    m_x1 = x1;
    m_y1 = y1;
    m_x2 = x2;
    m_y2 = y2;
    m_mask = mask;
    m_eventType = eventType;

  }

  void f()
  {
    // empty location stub
  }

  void Framework::EnableLocation(bool enable)
  {
//    if (enable)
//      m_work.StartLocationService(bind(&f));
//    else
//      m_work.StopLocationService();
  }

  void Framework::UpdateLocation(uint64_t timestamp, double lat, double lon, float accuracy)
  {
    location::GpsInfo info;
    info.m_timestamp = static_cast<double>(timestamp);
    info.m_latitude = lat;
    info.m_longitude = lon;
    info.m_horizontalAccuracy = accuracy;
//    info.m_status = location::EAccurateMode;
//    info.m_altitude = 0;
//    info.m_course = 0;
//    info.m_verticalAccuracy = 0;
    m_work.OnGpsUpdate(info);
  }

  void Framework::UpdateCompass(uint64_t timestamp, double magneticNorth, double trueNorth, float accuracy)
  {
    location::CompassInfo info;
    info.m_timestamp = static_cast<double>(timestamp);
    info.m_magneticHeading = magneticNorth;
    info.m_trueHeading = trueNorth;
    info.m_accuracy = accuracy;
    m_work.OnCompassUpdate(info);
  }

  void Framework::LoadState()
  {
    if (!m_work.LoadState())
    {
      LOG(LINFO, ("no saved state, showing all world"));
      m_work.ShowAll();
    }
    else
    {
      LOG(LINFO, ("state loaded successfully"));
    }
  }

  void Framework::SaveState()
  {
    m_work.SaveState();
  }
}
