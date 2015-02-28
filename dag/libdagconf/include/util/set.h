/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#ifndef SET_H
#define SET_H

/* Generic set that stores pointers.
 * The set makes no attempt to dispose of the elements it contains when it is disposed,
 * so don't call set_dispose() if the only reference you have to something is in the set.
 */

typedef struct Set_ Set_;
typedef Set_ * SetPtr;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


	SetPtr set_init(void);
	void set_dispose(SetPtr set);
	int set_get_count(SetPtr set);
	void set_add(SetPtr set, void* item);
	void* set_retrieve(SetPtr set, int index);
    void set_delete(SetPtr set, void* item);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SET_H */
