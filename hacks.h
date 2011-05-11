/*
 * hacks to cope with portability problems
 *
 * Copyright (C) 2011 Mark Smith <markzzzsmith@yahoo.com.au>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA. 
 */
#ifndef __HACKS_H
#define __HACKS_H

/*
 * Mac OS X doesn't have IPV6_ADD_MEMBERSHIP, IPV6_JOIN_GROUP is 
 * literal, legacy equivalent
 *
 */
#ifndef IPV6_ADD_MEMBERSHIP
#define IPV6_ADD_MEMBERSHIP	IPV6_JOIN_GROUP
#endif /* IPV6_ADD_MEMBERSHIP */

#endif /* __HACKS_H */

