
#ifndef __GST_MYFILTER_H__
#define __GST_MYFILTER_H__

#include <gst/gst.h>

G_BEGIN_DECLS


/* Definition of structure storing data for this element. */
typedef struct _GstMyFilter {
    GstElement element; //继承自 GstElement,此成员应作为class中第一个成员,且需要在 G_DEFINE_TYPE 中指明 GstElement 是父类

    GstPad *sinkpad, *srcpad;

    gboolean silent;
} GstMyFilter;

/* Standard definition defining a class for this element. */
typedef struct _GstMyFilterClass {
    GstElementClass parent_class;
} GstMyFilterClass;

/* Standard macros for defining types for this element.  */
#define GST_TYPE_MY_FILTER (gst_my_filter_get_type())

//强制转换为类型实例
#define GST_MY_FILTER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_MY_FILTER, GstMyFilter))
//强制转换为类型
#define GST_MY_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_MY_FILTER, GstMyFilterClass))

//判断是否是我们这个类型的实例
#define GST_IS_MY_FILTER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_MY_FILTER))
//判断是否是我们这个类型
#define GST_IS_MY_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_MY_FILTER))

/* Standard function returning type information. */
//G_DEFINE_TYPE 定义的时候是 gst_my_filter,所以这里只能是 gst_my_filter_get_type
//这个函数应该是 gstreamer 自动生成的, 这里声明出来就是供其他想继承本类的类型(需要 GST_TYPE_MY_FILTER)使用
GType gst_my_filter_get_type (void);

G_END_DECLS

#endif
