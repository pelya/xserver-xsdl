#ifndef __GLAMOR_DEBUG_H__
#define __GLAMOR_DEBUG_H__


#define GLAMOR_DELAYED_STRING_MAX 64

#define GLAMOR_DEBUG_NONE                     0
#define GLAMOR_DEBUG_UNIMPL                   0
#define GLAMOR_DEBUG_FALLBACK                 1
#define GLAMOR_DEBUG_TEXTURE_DOWNLOAD         2
#define GLAMOR_DEBUG_TEXTURE_DYNAMIC_UPLOAD   3

extern void
AbortServer(void)
    _X_NORETURN;

#define GLAMOR_PANIC(_format_, ...)			\
  do {							\
    LogMessageVerb(X_NONE, 0, "Glamor Fatal Error"	\
		   " at %32s line %d: " _format_ "\n",	\
		   __FUNCTION__, __LINE__,		\
		   ##__VA_ARGS__ );			\
    exit(1);                                            \
  } while(0)




#define __debug_output_message(_format_, _prefix_, ...) \
  LogMessageVerb(X_NONE, 0,				\
		 "%32s:\t" _format_ ,		\
		 /*_prefix_,*/				\
		 __FUNCTION__,				\
		 ##__VA_ARGS__)

#define glamor_debug_output(_level_, _format_,...)	\
  do {							\
    if (glamor_debug_level >= _level_)			\
      __debug_output_message(_format_,			\
			     "Glamor debug",		\
			     ##__VA_ARGS__);		\
  } while(0)


#define glamor_fallback(_format_,...)			\
  do {							\
    if (glamor_debug_level >= GLAMOR_DEBUG_FALLBACK)	\
      __debug_output_message(_format_,			\
			     "Glamor fallback",		\
			     ##__VA_ARGS__);} while(0)



#define glamor_delayed_fallback(_screen_, _format_,...)			\
  do {									\
    if (glamor_debug_level >= GLAMOR_DEBUG_FALLBACK)  {			\
      glamor_screen_private *_glamor_priv_;				\
      _glamor_priv_ = glamor_get_screen_private(_screen_);		\
      _glamor_priv_->delayed_fallback_pending = 1;			\
      snprintf(_glamor_priv_->delayed_fallback_string,			\
	       GLAMOR_DELAYED_STRING_MAX,				\
	       "glamor delayed fallback: \t%s " _format_ ,		\
               __FUNCTION__, ##__VA_ARGS__); } } while(0)


#define glamor_clear_delayed_fallbacks(_screen_)			\
  do {									\
    if (glamor_debug_level >= GLAMOR_DEBUG_FALLBACK) {			\
      glamor_screen_private *_glamor_priv_;				\
	_glamor_priv_ = glamor_get_screen_private(_screen_);		\
      _glamor_priv_->delayed_fallback_pending = 0;  } } while(0)

#define glamor_report_delayed_fallbacks(_screen_)			\
  do {									\
    if (glamor_debug_level >= GLAMOR_DEBUG_FALLBACK) {			\
      glamor_screen_private *_glamor_priv_;				\
      _glamor_priv_ = glamor_get_screen_private(_screen_);		\
      LogMessageVerb(X_INFO, 0, "%s",					\
		     _glamor_priv_->delayed_fallback_string);		\
      _glamor_priv_->delayed_fallback_pending = 0;  } } while(0)


#endif
