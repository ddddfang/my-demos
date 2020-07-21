
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include "gstmyfilter01.h"

GST_DEBUG_CATEGORY_STATIC (gst_my_filter_debug);
#define GST_CAT_DEFAULT gst_my_filter_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */

//static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
//    GST_PAD_SINK,
//    GST_PAD_ALWAYS,
//    GST_STATIC_CAPS (//caps string, 以 media type 打头, 然后是逗号分隔的 "properties with their supported values"
//        "audio/x-raw, "
//        "format = (string) " GST_AUDIO_NE (S16) ", "
//        "channels = (int) { 1, 2 }, "     //{} 表示集合
//        "rate = (int) [ 8000, 96000 ]")   //[] 表示范围
//    );

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

#define gst_my_filter_parent_class parent_class
//实例类型为 GstMyFilter, 类类型命名为 GstMyFilterClass, 
//后面的函数都将以 gst_my_filter 打头, 
//GST_TYPE_ELEMENT 是父类,且此成员应该位于类定义的第一个位置
G_DEFINE_TYPE (GstMyFilter, gst_my_filter, GST_TYPE_ELEMENT);



static void gst_my_filter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_my_filter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_my_filter_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_my_filter_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */

/* initialize the fangfilter's class */
static void
gst_my_filter_class_init (GstMyFilterClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_my_filter_set_property;
  gobject_class->get_property = gst_my_filter_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  //gst-inspect myfilter 时候会显示这些东西,看起来和 gst_element_class_set_static_metadata 等效
  //gst_element_class_set_details_simple(gstelement_class,
  //  "MyFilter",
  //  "Filter/Generic",
  //  "Create an experimental Template Element",
  //  "fang <<fridayfang@zhaoxin.com>>");
  gst_element_class_set_static_metadata (gstelement_class,
    "An example plugin",    //这是最主要的bref信息, gst-inspect |grep filter 时候会显示, gst-inspect myfilter 时候也会显示
    "Example/FirstExample",
    "Shows the basic structure of a plugin",
    "fang <<fridayfang@zhaoxin.com>>");

  //注册 pad template
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_my_filter_init (GstMyFilter * filter)
{
  //pad与实例相关 所以在这里配置
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_my_filter_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_my_filter_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->silent = FALSE;
}

static void
gst_my_filter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstMyFilter *filter = GST_MY_FILTER (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_my_filter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstMyFilter *filter = GST_MY_FILTER (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_my_filter_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstMyFilter *filter;
  gboolean ret;

  filter = GST_MY_FILTER (parent);

  GST_LOG_OBJECT (filter, "Received %s event: %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps * caps;

      gst_event_parse_caps (event, &caps);
      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_my_filter_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstMyFilter *filter;

  filter = GST_MY_FILTER (parent);

  if (filter->silent == FALSE)
    g_print ("I'm plugged, therefore I'm in.\n");

  /* just push out the incoming buffer without touching it */
  return gst_pad_push (filter->srcpad, buf);
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
myfilter_init (GstPlugin * myfilter)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template fangfilter' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_my_filter_debug, "myfilter",
      0, "Template myfilter");

  //gst_element_register 的第二个参数指出了本plugin的 elementfactory 名
  //遇到相同功能的plugin,gstreamer会优先选择rank数值大的plugin,gstreamer系统rank最大值 GST_RANK_PRIMARY(=256)
  //所以当你自己编写一个插件,希望gstreamer在使用你插件所具备的功能,在注册plugin的时候,将rank的值设置为比GST_RANK_PRIMARY大即可
  return gst_element_register (myfilter, "myfilter", GST_RANK_NONE,
      GST_TYPE_MY_FILTER);
}

/* PACKAGE: this is usually set by meson depending on some _INIT macro
 * in meson.build and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use meson to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstmyfilter"
#endif


//gst-inspect-1.0 |grep filter 显示的信息是:so名, elementfactory名, bref信息
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    myfilter,   //这个名字看起来不是随便起的,要有一个so名字就叫libgst<myfilter>.so, gstvideosinkdvb不受限制应该是和gstreamer版本有关系
    "Template myfilter",
    myfilter_init,
    PACKAGE_VERSION,
    GST_LICENSE,
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN
)
