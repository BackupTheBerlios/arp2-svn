/* bitmap.c */
BOOL bitmap_decompress(unsigned char *output, int width, int height,
		       unsigned char *input, int size, char bits);
/* cache.c */
HBITMAP cache_get_bitmap(uint8 cache_id, uint16 cache_idx);
void cache_put_bitmap(uint8 cache_id, uint16 cache_idx, HBITMAP bitmap);
FONTGLYPH *cache_get_font(uint8 font, uint16 character);
void cache_put_font(uint8 font, uint16 character, uint16 offset,
		    uint16 baseline, uint16 width, uint16 height,
		    HGLYPH pixmap);
DATABLOB *cache_get_text(uint8 cache_id);
void cache_put_text(uint8 cache_id, void *data, int length);
uint8 *cache_get_desktop(uint32 offset, int cx, int cy, int bytes_per_pixel);
void cache_put_desktop(uint32 offset, int cx, int cy, int scanline,
		       int bytes_per_pixel, uint8 * data);
HCOLOURMAP cache_get_colourmap(uint8 cache_id);
void cache_put_colourmap(uint8 cache_id, HCOLOURMAP data);
void cache_create (void);
void cache_destroy (void);
/* decompress.c */
int decompress(signed char *data, int clen, signed char ctype, signed char *dict, int *roff, int *rlen);
/* iso.c */
STREAM iso_init(int length);
void iso_send(STREAM s);
STREAM iso_recv(void);
BOOL iso_connect(char *server);
void iso_disconnect(void);
/* licence.c */
void licence_generate_keys(uint8 * client_key, uint8 * server_key,
			   uint8 * client_rsa);
void licence_process(STREAM s);
/* mcs.c */
STREAM mcs_init(int length);
void mcs_send(STREAM s);
STREAM mcs_recv(void);
BOOL mcs_connect(char *server, STREAM mcs_data);
void mcs_disconnect(void);
/* orders.c */
void process_orders(STREAM s);
void reset_order_state(void);
/* rdesktop.c */
int main(int argc, char *argv[]);
void generate_random(uint8 * random);
void *xmalloc(int size);
void *xrealloc(void *oldmem, int size);
void xfree(void *mem);
void error(char *format, ...);
void unimpl(char *format, ...);
void hexdump(unsigned char *p, unsigned int len);
/* rdp.c */
void rdp_out_unistr(STREAM s, char *string, int len);
void rdp_send_input(uint32 time, uint16 message_type, uint16 device_flags,
		    uint16 param1, uint16 param2);
void rdp_main_loop(void);
void rdp_send_palette ();
void rdp_send_rawrect ( int x, int y, int width, int height, char *rect,
                       int length);
void rdp_send_cpyrect ( int x_src, int y_src, int x_dest, int y_dest,
                       int width, int height );
void rdp_send_rect ( int x_dest, int y_dest, int width, int height, int colour );
BOOL rdp_connect(char *server, uint32 flags, char *domain, char *password,
		 char *command, char *directory);
void rdp_disconnect(void);
/* vnc.c */
int vnc_socket();
STREAM vnc_init (int maxlen);
void vnc_send (STREAM s);
STREAM vnc_recv (int length);
BOOL vnc_connect (char *server);
void vnc_disconnect ();
/* secure.c */
void sec_hash_48(uint8 * out, uint8 * in, uint8 * salt1, uint8 * salt2,
		 uint8 salt);
void sec_hash_16(uint8 * out, uint8 * in, uint8 * salt1, uint8 * salt2);
void buf_out_uint32(uint8 * buffer, uint32 value);
void sec_sign(uint8 * signature, uint8 * session_key, int length, uint8 * data,
	      int datalen);
STREAM sec_init(uint32 flags, int maxlen);
void sec_send(STREAM s, uint32 flags);
STREAM sec_recv(void);
BOOL sec_connect(char *server);
void sec_disconnect(void);
/* tcp.c */
int rdp_socket();
STREAM tcp_init(int maxlen);
void tcp_send(STREAM s);
STREAM tcp_recv(int length);
BOOL tcp_connect(char *server);
void tcp_disconnect(void);
/* xwin.c */
int ui_select_fd();
BOOL ui_create_window(char *title);
void ui_destroy_window(void);
void ui_process_events(void);
void ui_move_pointer(int x, int y);
HBITMAP ui_create_bitmap(int width, int height, uint8 * data);
HCURSOR cache_get_cursor(uint16 cache_idx);
void cache_put_cursor(uint16 cache_idx, HCURSOR cursor);
void ui_paint_bitmap(int x, int y, int cx, int cy, int width, int height,
		     uint8 * data);
void ui_destroy_bitmap(HBITMAP bmp);
HCURSOR ui_create_cursor(unsigned int x, unsigned int y, int width, int height,
			 uint8 * andmask, uint8 * xormask);
void ui_set_cursor(HCURSOR cursor);
void ui_destroy_cursor(HCURSOR cursor);
HGLYPH ui_create_glyph(int width, int height, uint8 * data);
void ui_destroy_glyph(HGLYPH glyph);
HCOLOURMAP ui_create_colourmap(COLOURMAP * colours);
void ui_destroy_colourmap(HCOLOURMAP map);
void ui_set_colourmap(HCOLOURMAP map);
void ui_set_clip(int x, int y, int cx, int cy);
void ui_reset_clip(void);
void ui_bell(void);
uint16 ui_get_toggle_keys(void);
void ui_destblt(uint8 opcode, int x, int y, int cx, int cy);
void ui_patblt(uint8 opcode, int x, int y, int cx, int cy, BRUSH * brush,
	       int bgcolour, int fgcolour);
void ui_screenblt(uint8 opcode, int x, int y, int cx, int cy, int srcx,
		  int srcy);
void ui_memblt(uint8 opcode, int x, int y, int cx, int cy, HBITMAP src, HCOLOURMAP map, int srcx, int srcy);
void ui_triblt(uint8 opcode, int x, int y, int cx, int cy, HBITMAP src, HCOLOURMAP map, int srcx, int srcy, BRUSH *brush, int bgcolour, int fgcolour);
void ui_line(uint8 opcode, int startx, int starty, int endx, int endy,
	     PEN * pen);
void ui_rect(int x, int y, int cx, int cy, int colour);
void ui_draw_glyph(int mixmode, int x, int y, int cx, int cy, HGLYPH glyph,
		   int srcx, int srcy, int bgcolour, int fgcolour, HBITMAP dst);
void ui_draw_text(uint8 font, uint8 flags, int mixmode, int x, int y, int clipx,
		  int clipy, int clipcx, int clipcy, int boxx, int boxy,
		  int boxcx, int boxcy, int bgcolour, int fgcolour,
		  uint8 * text, uint8 length);
void ui_desktop_save(uint32 offset, int x, int y, int cx, int cy);
void ui_desktop_restore(uint32 offset, int x, int y, int cx, int cy);
void ui_sync();
BOOL ui_pending();
/* keymap_wrap.c */
void init_keycodes(int id);

#ifdef SERVER
STREAM rfb_init(int length);
void rfb_send(STREAM s);
STREAM rfb_recv();
BOOL rfb_connect(char *server, char *password);
void rfb_disconnect();
BOOL rdp_listen(char *server, uint32 flags, char *domain, char *password,
		char *command, char *directory);
void rfb_send_keyevent ( uint32 keycode, uint32 downflag );
void rfb_send_pointerevent ( uint32 x, uint32 y, uint32 buttonmask );
void rfb_send_updaterequest ( uint16 x_off, uint16 y_off, uint16 width,
                             uint16 height, uint8 incremental );
 BOOL rdp_listen(char *server, uint32 flags, char *domain, char *password,
                char *command, char *directory);
void rdp_smain_loop ();
BOOL iso_listen(char *server);
BOOL sec_listen(char *server);
BOOL tcp_listen(char *server);
unsigned int x_key_to_sym( int key, int down );
BOOL mcs_listen(char *server, STREAM mcs_data, STREAM mcs_sdata);
#endif				/* SERVER */
