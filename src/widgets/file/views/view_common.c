#include "../../elementary_ext_priv.h"
#include "view_common.h"


static void
_file_remove(View_Common *common, Efm_File *file)
{
   Elm_Object_Item *it;

   it = eina_hash_find(common->files, &file);

   common->del(common, it);

   eina_hash_del(common->files, &file, it);
   view_file_unselect(common, file);
}

static void
_file_del(void *data, const Efl_Event *event)
{
   View_Common *common = data;
   Efm_File *file = event->object;

   _file_remove(common, file);
}

static void
_file_hide(void *data, const Efl_Event *event)
{
   View_Common *common = data;
   Efm_File *file = event->info;

   efl_event_callback_del(file, EFL_EVENT_DEL, _file_del, data);

   _file_remove(common, file);
}

static void
_file_add(void *data, const Efl_Event *event)
{
   View_Common *common = data;
   Efm_File *file = event->info;
   void *res;

   res = common->add(common, file);

   eina_hash_add(common->files, &file, res);
   efl_event_callback_add(file, EFM_FILE_EVENT_INVALID, _file_del, data);
}

void
_view_free(View_Common *common)
{
   if (common->monitor) efl_del(common->monitor);
   common->monitor = NULL;
   if (common->selection) eina_list_free(common->selection);
   common->selection = NULL;
   if (common->files) eina_hash_free(common->files);
   common->files = NULL;
}

static void
_error(void *data, const Efl_Event *event EINA_UNUSED)
{
   View_Common *common = data;

   _view_free(common);

   common->err(common);

   efl_event_callback_call(common->obj, ELM_FILE_VIEW_EVENT_ITEM_SELECT_CHANGED, NULL);
}

static void
_listing_done(void *data, const Efl_Event *event EINA_UNUSED
  )
{
   View_Common *common = data;

   efl_event_callback_call(common->obj, ELM_FILE_VIEW_EVENT_WORKING_DONE , NULL);
}

EFL_CALLBACKS_ARRAY_DEFINE(_monitor_event_cbs,
  {EFM_MONITOR_EVENT_FILE_ADD, _file_add},
  {EFM_MONITOR_EVENT_FILE_HIDE, _file_hide},
  {EFM_MONITOR_EVENT_ERROR, _error},
  {EFM_MONITOR_EVENT_LISTING_DONE, _listing_done}
);

void
view_common_init(View_Common *common, Eo *obj, Item_Add_Cb add, Item_Del_Cb del, Error_Cb err, Item_Select_Cb sel)
{
   memset(common, 0, sizeof(View_Common));

   common->obj = obj;
   common->add = add;
   common->del = del;
   common->err = err;
   common->sel = sel;
}

void
view_file_select(View_Common *common, Efm_File *f)
{
   Elm_Object_Item *it;

   it = eina_hash_find(common->files, &f);
   common->sel(common, it, EINA_TRUE);

   common->selection = eina_list_append(common->selection, f);
   efl_event_callback_call(common->obj, ELM_FILE_VIEW_EVENT_ITEM_SELECT_CHANGED, common->selection);
}

void
view_file_unselect(View_Common *common, Efm_File *f)
{
   Elm_Object_Item *it;

   it = eina_hash_find(common->files, &f);
   common->sel(common, it, EINA_FALSE);

   common->selection = eina_list_remove(common->selection, f);
   efl_event_callback_call(common->obj, ELM_FILE_VIEW_EVENT_ITEM_SELECT_CHANGED, common->selection);
}

void
view_file_set(View_Common *common, Efm_File *file)
{
   _view_free(common);

   efl_event_callback_call(common->obj, ELM_FILE_VIEW_EVENT_ITEM_SELECT_CHANGED, common->selection);
   common->files = eina_hash_pointer_new(NULL);

   efl_event_callback_call(common->obj, ELM_FILE_VIEW_EVENT_WORKING_START , NULL);
   efl_del(common->monitor);
   common->monitor = efm_file_monitor(file, common->f);
   efl_event_callback_array_add(common->monitor, _monitor_event_cbs(), common);
}

void
view_filter_set(View_Common *common, Efm_Filter *filter)
{
   common->f = filter;
   if (common->monitor)
     efm_monitor_filter_set(common->monitor, filter);
}

Efm_File*
view_search(View_Common *common, const char *search)
{
   Eina_Iterator *itr;
   Efm_File **file, *searched = NULL;
   int min = -1;

   if (!common->files) return NULL;

   itr = eina_hash_iterator_key_new(common->files);

   EINA_ITERATOR_FOREACH(itr, file)
     {
        const char *filename;
        char *f;

        filename = efm_file_filename_get(*file);
        if ((f = strstr(filename, search)))
          {
             int tmin = f - filename;
             if (min == -1)
               min = tmin;

             if (tmin > min)
               continue;
             min = tmin;
             searched = *file;
          }
     }

   if (searched)
     return eina_hash_find(common->files, &searched);
   else
     return NULL;
}