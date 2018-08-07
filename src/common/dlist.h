/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Based on apr_ring.h
 */

#ifndef __NX_DLIST_H
#define __NX_DLIST_H

#include "error_debug.h"

#define NX_DLIST_ENTRY(elem)						\
    struct {								\
	struct elem * volatile next;					\
	struct elem * volatile prev;					\
    }

#define NX_DLIST_HEAD(head, elem)					\
    struct head {							\
	struct elem *first;						\
	struct elem *last;						\
    }

#define NX_DLIST_FIRST(hp)	(hp)->first

#define NX_DLIST_LAST(hp)	(hp)->last

#define NX_DLIST_NEXT(ep, link)	(ep)->link.next

#define NX_DLIST_PREV(ep, link)	(ep)->link.prev

#define NX_DLIST_INIT(hp, elem, link) do {				\
	NX_DLIST_FIRST((hp)) = NULL;					\
	NX_DLIST_LAST((hp))  = NULL;					\
    } while (0)

#define NX_DLIST_EMPTY(hp)						\
    (((NX_DLIST_FIRST((hp)) == NULL) && (NX_DLIST_LAST((hp)) == NULL)) ? TRUE : FALSE)

#define NX_DLIST_INSERT_BEFORE(hp, lep, nep, link)			\
    (nep)->link.next = lep;						\
    if ( (lep)->link.prev == NULL ) (hp)->first = nep;			\
    else (lep)->link.prev->link.next = nep;                             \
    (nep)->link.prev = lep->link.prev;					\
    (lep)->link.prev = nep

#define NX_DLIST_INSERT_AFTER(hp, lep, nep, link)			\
    (nep)->link.prev = lep;						\
    if ( (lep)->link.next == NULL ) (hp)->last = nep;			\
    (nep)->link.next = lep->link.next;					\
    (lep)->link.next = nep

#define NX_DLIST_INSERT_HEAD(hp, nep, link)				\
    (nep)->link.prev = NULL;						\
    (nep)->link.next = (hp)->first;					\
    if ( (hp)->first != NULL ) (hp)->first->link.prev = nep;		\
    (hp)->first = nep;							\
    if ( (hp)->last == NULL ) (hp)->last = nep

#define NX_DLIST_INSERT_TAIL(hp, nep, link)				\
    (nep)->link.next = NULL;						\
    (nep)->link.prev = (hp)->last;					\
    if ( (hp)->last != NULL ) (hp)->last->link.next = nep;		\
    (hp)->last = nep;							\
    if ( (hp)->first == NULL ) (hp)->first = nep

#define NX_DLIST_REMOVE(hp, ep, link)					\
    if ( (hp)->first == ep ) (hp)->first = (hp)->first->link.next;	\
    else (ep)->link.prev->link.next = (ep)->link.next; 			\
    if ( (hp)->last == ep ) (hp)->last = (hp)->last->link.prev;		\
    else (ep)->link.next->link.prev = (ep)->link.prev;			\
    (ep)->link.prev = NULL;						\
    (ep)->link.next = NULL		

#define NX_DLIST_CHECK(hp, link)					\
    if ( (hp)->first == NULL ) ASSERT((hp)->last == NULL);		\
    else 								\
    {									\
         ASSERT((hp)->first->link.prev == NULL);			\
         ASSERT((hp)->last->link.next == NULL);				\
    }


#endif /* __NX_DLIST_H */
