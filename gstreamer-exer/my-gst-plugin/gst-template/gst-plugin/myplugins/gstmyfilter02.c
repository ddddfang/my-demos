
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include "gstmyfilter02.h"

GST_DEBUG_CATEGORY_STATIC (gst_my_filter_debug);
#define GST_CAT_DEFAULT gst_my_filter_debug

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

G_DEFINE_TYPE (GstMyFilter, gst_my_filter, GST_TYPE_ELEMENT);



static void gst_my_filter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_my_filter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_my_filter_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static gboolean gst_my_filter_src_query (GstPad * pad, GstObject *parent, GstQuery  *query);
static GstFlowReturn gst_my_filter_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);
static GstStateChangeReturn gst_my_filter_change_state (GstElement *element, GstStateChange transition);

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

  gst_element_class_set_static_metadata (gstelement_class,
    "An example plugin",
    "Example/FirstExample",
    "Shows the basic structure of a plugin",
    "fang <<fridayfang@zhaoxin.com>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));

  //change_state 是父类提供的虚函数接口,这里重载它
  gstelement_class->change_state = gst_my_filter_change_state;
}

static void
gst_my_filter_init (GstMyFilter * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_my_filter_sink_event)); //sink/src pad 都可以接收 caps, end-of-stream, newsegment, tags 等 stream-events
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_my_filter_chain));      //chain 函数负责处理 sinkpad 接收的数据
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad); //?
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  gst_pad_set_query_function (filter->srcpad,
                              gst_my_filter_src_query); //sink/src pad 都可以处理一些query,用于回应 对本element的查询(position, duration ,supported formats, scheduling modes 等)
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);  //?
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

      ret = gst_pad_event_default (pad, parent, event);
      ///* push the event downstream */
      //ret = gst_pad_push_event (filter->srcpad, event);
      break;
    }
    case GST_EVENT_EOS:
      /* end-of-stream, we should close down all stream leftovers here */

      ret = gst_pad_event_default (pad, parent, event);
      break;
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

//Through the query function, your element will receive queries that it has to reply to. 
//These are queries like position, duration but also about the supported formats and scheduling modes your element supports.
//Queries can travel both upstream and downstream, so you can receive them on sink pads as well as source pads.
static gboolean
gst_my_filter_src_query (GstPad * pad, GstObject *parent, GstQuery  *query)
{
  gboolean ret;
  GstMyFilter *filter = GST_MY_FILTER (parent);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_POSITION:
      /* we should report the current position */

      ret = gst_pad_query_default (pad, parent, query);
      break;
    case GST_QUERY_DURATION:
      /* we should report the duration here */
      
      ret = gst_pad_query_default (pad, parent, query);
      break;
    case GST_QUERY_CAPS:
      /* we should report the supported caps here */
      
      ret = gst_pad_query_default (pad, parent, query);
      break;
    default:
      /* just call the default handler */
      ret = gst_pad_query_default (pad, parent, query);
      break;
  }
  return ret;
}

static GstFlowReturn
gst_my_filter_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstMyFilter *filter;

  filter = GST_MY_FILTER (parent);

  if (filter->silent == FALSE)
    g_print ("Have data of size %" G_GSIZE_FORMAT " bytes!\n", gst_buffer_get_size (buf));

  return gst_pad_push (filter->srcpad, buf);

  //GstMyFilter *filter = GST_MY_FILTER (parent);
  //GstBuffer *outbuf;

  //outbuf = gst_my_filter_process_data (filter, buf);
  //gst_buffer_unref (buf);
  //if (!outbuf) {
  //  /* something went wrong - signal an error */
  //  GST_ELEMENT_ERROR (GST_ELEMENT (filter), STREAM, FAILED, (NULL), (NULL));
  //  return GST_FLOW_ERROR;
  //}

  //return gst_pad_push (filter->srcpad, outbuf);
}

//GST_STATE_NULL
//  是 element 的默认状态. 未分配任何 runtime 资源, 也没有加载任何 runtime libraries.
//GST_STATE_READY
//  分配了默认的资源 (runtime-libraries, runtime-memory) 但 stream 相关的资源尚未分配. 文件算是 stream-specific 资源
//GST_STATE_PAUSED
//  对大部分 elements 来说此 state 等同于 PLAYING,会接收并处理data. 但 Sink elements 在此状态只接收一个 buffer 然后 block.
//  这就叫 pipeline is 'prerolled' 然后后面转入PLAYING就可以 render data immediately 了.
//GST_STATE_PLAYING
//  接收并处理 buffers/events , sink elements 在此状态真实的 render incoming data
//  e.g. output audio to a sound card or render video pictures to an image sink.
static GstStateChangeReturn
gst_my_filter_change_state (GstElement *element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstMyFilter *filter = GST_MY_FILTER (element);
  
  g_print ("change_state in: %s => %s\n",
    gst_element_state_get_name(GST_STATE_TRANSITION_CURRENT(transition)),
    gst_element_state_get_name(GST_STATE_TRANSITION_NEXT(transition)));

  //g_print ("change_state in: %s => %s\n",
  //  gst_element_state_get_name((GstState)((transition >> 3) & 0x07)),
  //  gst_element_state_get_name((GstState)(transition & 0x07)));

  switch (transition) { //状态升序放在 parent_class change_state 前
    case GST_STATE_CHANGE_NULL_TO_READY:
      //if (!gst_my_filter_allocate_memory (filter))
      //  return GST_STATE_CHANGE_FAILURE;
      break;
    default:
      break;
  }
  
  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;
  
  switch (transition) { //状态降序放在 parent_class change_state 后, This is necessary in order to safely handle concurrent access by multiple threads.
    case GST_STATE_CHANGE_READY_TO_NULL:
      //gst_my_filter_free_memory (filter);
      break;
    default:
      break;
  }
  
  return ret;
}


static gboolean
myfilter_init (GstPlugin * myfilter)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template fangfilter' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_my_filter_debug, "myfilter",
      0, "Template myfilter");

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
    myfilter,
    "Template myfilter",
    myfilter_init,
    PACKAGE_VERSION,
    GST_LICENSE,
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN
)
