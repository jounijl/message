/* 
 * Functions to check JSON values. 
 * 
 * Copyright (C) 2015 and 2016. Jouni Laakso
 *
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * Otherwice, this library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser
 * General Public License version 2.1 as published by the Free Software Foundation 6. of June year 2012;
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
 * more details. You should have received a copy of the GNU Lesser General Public License along with this library; if
 * not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * licence text is in file LIBRARY_LICENCE.TXT with a copyright notice of the licence text.
 */

/* cb_check_json_string(  unsigned char **ucsname, int namelength, int *from  ) is in cb_buffer.h */

int  cb_check_json_value( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from );

int  cb_check_json_number( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from );
int  cb_check_json_array( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from );
int  cb_check_json_object( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from );
int  cb_check_json_true( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from );
int  cb_check_json_false( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from );
int  cb_check_json_null( CBFILE **cfg, unsigned char **ucsvalue, int ucsvaluelen, int *from );
