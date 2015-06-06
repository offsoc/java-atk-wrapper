/*
 * Java ATK Wrapper for GNOME
 * Copyright (C) 2009 Sun Microsystems Inc.
 * Copyright (C) 2015 Magdalen Berns <m.berns@thismagpie.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <atk/atk.h>
#include <glib.h>
#include "jawobject.h"
#include "jawwindow.h"
#include "jawutil.h"
#include "jawtoplevel.h"

static void jaw_object_class_init(JawObjectClass *klass);
static void jaw_object_init(JawObject *object);

/* AtkObject */
static const gchar* jaw_object_get_name(AtkObject *atk_obj);
static const gchar* jaw_object_get_description(AtkObject *atk_obj);

static gint jaw_object_get_n_children(AtkObject *atk_obj);

static gint jaw_object_get_index_in_parent(AtkObject *atk_obj);

static AtkRole jaw_object_get_role(AtkObject *atk_obj);
static AtkStateSet* jaw_object_ref_state_set(AtkObject *atk_obj);

static gpointer parent_class = NULL;

G_DEFINE_TYPE (JawObject, jaw_object, ATK_TYPE_OBJECT);

static void
jaw_object_class_init (JawObjectClass *klass)
{
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);

  atk_class->get_name = jaw_object_get_name;
  atk_class->get_description = jaw_object_get_description;
  atk_class->get_n_children = jaw_object_get_n_children;
  atk_class->get_index_in_parent = jaw_object_get_index_in_parent;
  atk_class->get_role = jaw_object_get_role;
  atk_class->get_layer = NULL;
  atk_class->get_mdi_zorder = NULL;
  atk_class->ref_state_set = jaw_object_ref_state_set;
/*	atk_class->set_name = jaw_object_set_name;
	atk_class->set_description = jaw_object_set_description;
	atk_class->set_parent = jaw_object_set_parent;
	atk_class->set_role = jaw_object_set_role;
	atk_class->connect_property_change_handler = jaw_object_connect_property_change_handler;
	atk_class->remove_property_change_handler = jaw_object_remove_property_change_handler;
	atk_class->children_changed = jaw_object_children_changed;
	atk_class->focus_event = jaw_object_focus_event;
	atk_class->property_change = jaw_object_property_change;
	atk_class->state_change = jaw_object_state_change;
	atk_class->visible_data_changed = jaw_object_visible_data_changed;
	atk_class->active_descendant_changed = jaw_object_active_descendant_changed;
	atk_class->get_attributes = jaw_object_get_attributes;
*/
  klass->get_interface_data = NULL;
}

gpointer
jaw_object_get_interface_data (JawObject *jaw_obj, guint iface)
{
  JawObjectClass *klass = JAW_OBJECT_GET_CLASS(jaw_obj);
  if (klass->get_interface_data)
    return klass->get_interface_data(jaw_obj, iface);

  return NULL;
}

static void
jaw_object_init (JawObject *object)
{
  AtkObject *atk_obj = ATK_OBJECT(object);
  atk_obj->description = NULL;

  object->state_set = atk_state_set_new();
}

static const gchar*
jaw_object_get_name (AtkObject *atk_obj)
{
  JawObject *jaw_obj = JAW_OBJECT(atk_obj);
  jobject ac = jaw_obj->acc_context;
  JNIEnv *jniEnv = jaw_util_get_jni_env();

  atk_obj->name = (gchar *)ATK_OBJECT_CLASS (parent_class)->get_name (atk_obj);

  if (atk_object_get_role(atk_obj) == ATK_ROLE_COMBO_BOX &&
      atk_object_get_n_accessible_children(atk_obj) == 1)
  {
    AtkSelection *selection = ATK_SELECTION(atk_obj);
    if (selection != NULL)
    {
      AtkObject *child = atk_selection_ref_selection(selection, 0);
      if (child != NULL)
      {
        return atk_object_get_name(child);
      }
    }
  }

  jclass classAccessibleContext = (*jniEnv)->FindClass(jniEnv,
                                                       "javax/accessibility/AccessibleContext" );
  jmethodID jmid = (*jniEnv)->GetMethodID(jniEnv,
                                          classAccessibleContext,
                                          "getAccessibleName",
                                          "()Ljava/lang/String;");
  jstring jstr = (*jniEnv)->CallObjectMethod( jniEnv, ac, jmid );

  if (atk_obj->name != NULL)
  {
    (*jniEnv)->ReleaseStringUTFChars(jniEnv, jaw_obj->jstrName, atk_obj->name);
    (*jniEnv)->DeleteGlobalRef(jniEnv, jaw_obj->jstrName);
  }

  if (jstr != NULL)
  {
    jaw_obj->jstrName = (*jniEnv)->NewGlobalRef(jniEnv, jstr);
    atk_obj->name = (gchar*)(*jniEnv)->GetStringUTFChars(jniEnv,
                                                         jaw_obj->jstrName,
                                                         NULL);
  }

  return atk_obj->name;
}

static const gchar*
jaw_object_get_description (AtkObject *atk_obj)
{
  JawObject *jaw_obj = JAW_OBJECT(atk_obj);
  jobject ac = jaw_obj->acc_context;
  JNIEnv *jniEnv = jaw_util_get_jni_env();

  jclass classAccessibleContext = (*jniEnv)->FindClass( jniEnv,
                                                       "javax/accessibility/AccessibleContext" );
  jmethodID jmid = (*jniEnv)->GetMethodID(jniEnv,
                                          classAccessibleContext,
                                          "getAccessibleDescription",
                                          "()Ljava/lang/String;");
  jstring jstr = (*jniEnv)->CallObjectMethod( jniEnv, ac, jmid );

  if (atk_obj->description != NULL)
  {
    (*jniEnv)->ReleaseStringUTFChars(jniEnv, jaw_obj->jstrDescription, atk_obj->description);
    (*jniEnv)->DeleteGlobalRef(jniEnv, jaw_obj->jstrDescription);
    atk_obj->description = NULL;
  }

  if (jstr != NULL)
  {
    jaw_obj->jstrDescription = (*jniEnv)->NewGlobalRef(jniEnv, jstr);
    atk_obj->description = (gchar*)(*jniEnv)->GetStringUTFChars(jniEnv,
                                                                jaw_obj->jstrDescription,
                                                                NULL);
  }

  return atk_obj->description;
}

static gint
jaw_object_get_n_children (AtkObject *atk_obj)
{
  JawObject *jaw_obj = JAW_OBJECT(atk_obj);
  jobject ac = jaw_obj->acc_context;
  JNIEnv *jniEnv = jaw_util_get_jni_env();

  jclass classAccessibleContext = (*jniEnv)->FindClass(jniEnv,
                                                       "javax/accessibility/AccessibleContext" );
  jmethodID jmid = (*jniEnv)->GetMethodID(jniEnv,
                                          classAccessibleContext,
                                          "getAccessibleChildrenCount",
                                          "()I");
  jint count = (*jniEnv)->CallIntMethod( jniEnv, ac, jmid );

  return (gint)count;
}

static gint
jaw_object_get_index_in_parent (AtkObject *atk_obj)
{
  if (jaw_toplevel_get_child_index(JAW_TOPLEVEL(atk_get_root()), atk_obj) != -1)
  {
    return jaw_toplevel_get_child_index(JAW_TOPLEVEL(atk_get_root()), atk_obj);
  }

  JawObject *jaw_obj = JAW_OBJECT(atk_obj);
  jobject ac = jaw_obj->acc_context;
  JNIEnv *jniEnv = jaw_util_get_jni_env();

  jclass classAccessibleContext = (*jniEnv)->FindClass(jniEnv,
                                                       "javax/accessibility/AccessibleContext" );
  jmethodID jmid = (*jniEnv)->GetMethodID(jniEnv,
                                          classAccessibleContext,
                                          "getAccessibleIndexInParent",
                                          "()I");
  jint index = (*jniEnv)->CallIntMethod( jniEnv, ac, jmid );

  return (gint)index;
}

static AtkRole
jaw_object_get_role (AtkObject *atk_obj)
{
  JawObject *jaw_obj = JAW_OBJECT(atk_obj);
  atk_obj->role = jaw_util_get_atk_role_from_jobj(jaw_obj->acc_context);
  return atk_obj->role;
}

static AtkStateSet*
jaw_object_ref_state_set (AtkObject *atk_obj)
{
  JawObject *jaw_obj = JAW_OBJECT(atk_obj);
  AtkStateSet* state_set = jaw_obj->state_set;
  atk_state_set_clear_states( state_set );

  jobject ac = jaw_obj->acc_context;
  JNIEnv *jniEnv = jaw_util_get_jni_env();
  jclass classAccessibleContext = (*jniEnv)->FindClass(jniEnv,
                                                       "javax/accessibility/AccessibleContext" );
  jmethodID jmid = (*jniEnv)->GetMethodID(jniEnv,
                                          classAccessibleContext,
                                          "getAccessibleStateSet",
                                          "()Ljavax/accessibility/AccessibleStateSet;" );
  jobject jstate_set = (*jniEnv)->CallObjectMethod( jniEnv, ac, jmid );

  jclass classAccessibleStateSet = (*jniEnv)->FindClass(jniEnv,
                                                        "javax/accessibility/AccessibleStateSet" );
  jmid = (*jniEnv)->GetMethodID(jniEnv,
                                classAccessibleStateSet,
                                "toArray",
                                "()[Ljavax/accessibility/AccessibleState;");

  jobjectArray jstate_arr = (*jniEnv)->CallObjectMethod( jniEnv, jstate_set, jmid );

  jsize jarr_size = (*jniEnv)->GetArrayLength(jniEnv, jstate_arr);
  jsize i;
  for (i = 0; i < jarr_size; i++)
  {
    jobject jstate = (*jniEnv)->GetObjectArrayElement( jniEnv, jstate_arr, i );
    AtkStateType state_type = jaw_util_get_atk_state_type_from_java_state( jniEnv, jstate );
    atk_state_set_add_state( state_set, state_type );
    if (state_type == ATK_STATE_ENABLED)
    {
      atk_state_set_add_state( state_set, ATK_STATE_SENSITIVE );
    }
  }

  if (G_OBJECT(state_set) != NULL)
    g_object_ref(G_OBJECT(state_set));

  return state_set;
}

