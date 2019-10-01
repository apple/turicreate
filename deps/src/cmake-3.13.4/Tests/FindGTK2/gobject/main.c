/* Taken from https://developer.gnome.org/gobject/stable/chapter-gobject.html
 */

#include <glib-object.h>

#define MAMAN_TYPE_BAR (maman_bar_get_type())
#define MAMAN_BAR(obj)                                                        \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), MAMAN_TYPE_BAR, MamanBar))
#define MAMAN_IS_BAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), MAMAN_TYPE_BAR))
#define MAMAN_BAR_CLASS(klass)                                                \
  (G_TYPE_CHECK_CLASS_CAST((klass), MAMAN_TYPE_BAR, MamanBarClass))
#define MAMAN_IS_BAR_CLASS(klass)                                             \
  (G_TYPE_CHECK_CLASS_TYPE((klass), MAMAN_TYPE_BAR))
#define MAMAN_BAR_GET_CLASS(obj)                                              \
  (G_TYPE_INSTANCE_GET_CLASS((obj), MAMAN_TYPE_BAR, MamanBarClass))

typedef struct _MamanBar MamanBar;
typedef struct _MamanBarClass MamanBarClass;

struct _MamanBar
{
  GObject parent_instance;

  /* instance members */
};

struct _MamanBarClass
{
  GObjectClass parent_class;

  /* class members */
};

/* will create maman_bar_get_type and set maman_bar_parent_class */
G_DEFINE_TYPE(MamanBar, maman_bar, G_TYPE_OBJECT);

static GObject* maman_bar_constructor(GType gtype, guint n_properties,
                                      GObjectConstructParam* properties)
{
  GObject* obj;

  {
    /* Always chain up to the parent constructor */
    obj = G_OBJECT_CLASS(maman_bar_parent_class)
            ->constructor(gtype, n_properties, properties);
  }

  /* update the object state depending on constructor properties */

  return obj;
}

static void maman_bar_class_init(MamanBarClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->constructor = maman_bar_constructor;
}

static void maman_bar_init(MamanBar* self)
{
  /* initialize the object */
}

int main(int argc, char* argv[])
{
  MamanBar* bar = g_object_new(MAMAN_TYPE_BAR, NULL);
  g_object_unref(bar);
  return 0;
}
