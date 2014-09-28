#include "user.h"
#include "conf.h"
#include "tool.h"
#include "virtual_file.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/fcntl.h>	

//////////////////////////////////////////////
// user index


void toy_avltree_free(struct toy_node_s *ptr)
{
	if(ptr->left)
		toy_avltree_free(ptr->left);
	if(ptr->right)
		toy_avltree_free(ptr->right);
	free(ptr->user_name);
	free(ptr);
}


void toy_index_destroy(struct toy_index_s *ptr)
{
	if(ptr->root)
		toy_avltree_free(ptr->root);
	free(ptr);
}

const struct toy_node_s* toy_index_search(const struct toy_index_s *index, const char *user_name)
{
	struct toy_node_s *ptr = index->root;
	int cmp_ret;
	while(ptr != NULL)
	{
		cmp_ret = strcmp(user_name, ptr->user_name);
		if( cmp_ret < 0)
			ptr = ptr->left;
		else if( cmp_ret > 0)
			ptr = ptr->right;
		else
			return ptr;
	}
	return ptr;
}

/*
void toy_avltree_add(struct toy_node_s *root, struct toy_node_s *new_node)
{
	int cmp_ret = strcmp(new_node->user_name, root->user_name);
	if(cmp_ret < 0)
	{
		if(root->left == NULL)
		{
			root->left = new_node;
			root->height = 1;			// root->right->height must be <= 0
			return ;
		}
		else
		{
			toy_avltree_add(root->left, new_node);
			if(root->right == NULL)
				root->height = root->left->height + 1;
			else
				root->height = toy_max(root->left->height, root->right->height) + 1;
		}
	}
	else if(cmp_ret > 0)
	{
		if(root->right == NULL)
		{
			root->right = new_node;
			root->height = 1;
			return ;
		}
		else
		{
			toy_avltree_add(root->right, new_node);
			if(root->left == NULL)
				root->height = root->right->height + 1;
			else
				root->height = toy_max(root->left->height, root->right->height) + 1;
		}
	}
	else
		return ;

	int balance = root->left->height - root->right->height;
	if(balance <= -2 || balance >= 2)
		toy_avltree_
}*/

struct toy_node_s *toy_avltree_balance(struct toy_node_s *root)
{
	struct toy_node_s *new_root;
	if(root->balance == -2 && root->left->balance == -1)
	{
		new_root = root->left;
		root->left = new_root->right;
		new_root->right = root;

		new_root->right->balance = 0;
		new_root->balance = 0;
	}
	else if(root->balance == -2 && root->left->balance == 1)
	{
		new_root = root->left->right;
		root->left->right = new_root->left;
		new_root->left = root->left;
		root->left = new_root->right;
		new_root->right = root;

		if(new_root->balance == -1)
		{
			new_root->left->balance = 0;
			new_root->right->balance = 1;
		}
		else if(new_root->balance == 1)
		{
			new_root->left->balance = -1;
			new_root->right->balance = 0;
		}
		else
		{
			new_root->right->balance = 0;
			new_root->left->balance = 0;
		}
	}
	else if(root->balance == 2 && root->right->balance == 1)
	{
		new_root = root->right;
		root->right = new_root->left;
		new_root->left = root;

		new_root->left->balance = 0;
		new_root->balance = 0;
	}
	else
	{
		new_root = root->right->left;
		root->right->left = new_root->right;
		new_root->right = root->right;
		root->right = new_root->left;
		new_root->left = root;

		if(new_root->balance == 1)
		{
			new_root->right->balance = 0;
			new_root->left->balance = -1;
		}
		else if(new_root->balance == -1)
		{
			new_root->right->balance = 1;
			new_root->left->balance = 0;
		}
		else
		{
			new_root->right->balance = 0;
			new_root->left->balance = 0;
		}
	}
	new_root->balance = 0;
	return new_root;
}

struct toy_node_s * toy_avltree_add(struct toy_node_s *root, struct toy_node_s *new_node)
{
	struct toy_pathnode_s 
	{
		struct toy_node_s *p_node;
		struct toy_pathnode_s *p_next;
	} ;
	struct toy_pathnode_s *head = NULL;
	struct toy_pathnode_s *ptemp = NULL;

	struct toy_node_s *p_cur = root;
	int cmp_ret;
	while(p_cur)
	{
		cmp_ret = strcmp(new_node->user_name, p_cur->user_name);
		if(cmp_ret == 0)
			break ;
		ptemp = malloc(sizeof(struct toy_pathnode_s));
		ptemp->p_node = p_cur;
		ptemp->p_next = head;
		head = ptemp;
		if(cmp_ret < 0)
			p_cur = p_cur->left;
		else
			p_cur = p_cur->right;
	}
	if(p_cur == NULL)			//succeed
	{
		ptemp = head;
		p_cur = ptemp->p_node;
		if(strcmp(new_node->user_name, p_cur->user_name) < 0)
		{
			p_cur->left = new_node;
			p_cur->balance += -1;
		}
		else
		{
			p_cur->right = new_node;
			p_cur->balance += 1;
		}

/*		int height = 1;
		while(ptemp)
		{
			if(height == ptemp->p_node->height)
				break;
			ptemp->p_node->height = height;
			if((ptemp->p_node->left == NULL || ptemp->p_node->right == NULL) && ptemp->p_node->height == 2)
			{
				toy_avltree_balance(ptemp->p_next->p_node);
				break;
			}
			int balance = ptemp->p_node->left->height - ptemp->p_node->right->height;
			if(balance == -2 || balance == 2)
			{
				toy_avltree_balance(ptemp->p_next->p_node);
				break;
			}
			++ height;
			ptemp = ptemp->p_next;
		}*/
		struct toy_node_s *p_parent;
		for(;;)
		{
			p_cur = ptemp->p_node;
			if(ptemp->p_next == NULL)
				p_parent = NULL;
			else
				p_parent = ptemp->p_next->p_node;

			if(p_cur->balance == 0)
				break;
			if(p_cur->balance == 2 || p_cur->balance == -2)
			{
				if(p_parent == NULL)
					root = toy_avltree_balance(p_cur);
				else if(p_parent->left == p_cur)
					p_parent->left = toy_avltree_balance(p_cur);
				else
					p_parent->right = toy_avltree_balance(p_cur);
				break;
			}
			if(p_parent == NULL)
				break;
			if(p_parent->left == p_cur)
				p_parent->balance += -1;
			else
				p_parent->balance += 1;

			ptemp = ptemp->p_next;
		}
	}
	while(head)
	{
		ptemp = head;
		head = head->p_next;
		free(ptemp);
	}
	return root;
}

int toy_index_add(struct toy_index_s *index, struct toy_node_s *new_node)
{
	if(toy_index_search(index, new_node->user_name) != NULL)
		return 0;

	if(index->root == NULL)
	{
		index->root = new_node;
		return 1;
	}
	index->root = toy_avltree_add(index->root, new_node);
}


///////////////////////////////////////////
// 

struct toy_user_s toy_user_global;

void toy_user_init_infos()
{
	int fd = open(toy_conf_global.option_userinfos_file, O_RDWR);
	if(fd == -1)
		toy_kill("open file error on toy_user_init_infos");
	struct stat stat_buf;
	fstat(fd, &stat_buf);
	if(stat_buf.st_size == 0)
	{
		toy_user_global.users_num = 0;
		toy_user_global.users_index.root = NULL;
		return ;
	}
	read(fd, &toy_user_global.users_num, sizeof(u_int));
	toy_user_global.users_index.root = NULL;

	u_int i;
	u_int name_size;
	struct toy_node_s *new_node;
	for(i = 0; i < toy_user_global.users_num; ++i)
	{
		read(fd, &name_size, sizeof(u_int));
		new_node = malloc(sizeof(struct toy_node_s));
		new_node->user_name = malloc(name_size);
		read(fd, new_node->user_name, name_size);
		read(fd, &new_node->user_id, sizeof(u_int));
		new_node->left = NULL;
		new_node->right = NULL;
		new_node->balance = 0;
		toy_index_add(&toy_user_global.users_index, new_node);
	}
	close(fd);
}


int toy_user_get_id_by_name(const char *user_name, u_int *id_buf)
{
	const struct toy_node_s *p_user = toy_index_search(&toy_user_global.users_index, user_name);
	if(p_user == NULL)
		return 0;
	*id_buf = p_user->user_id;
	return 1;
}

int toy_user_check_pwd(u_int user_id, const u_char *pwdhash)
{
	int fd = open(toy_conf_global.option_pwdhashs_file, O_RDONLY);
	if(fd == -1)
		toy_kill("failed open file on toy_user_check_pwd()");
	lseek(fd, user_id * 16, SEEK_SET);
	u_char localhash[16];
	read(fd, localhash, 16);
	int retval;
	if(memcmp(pwdhash, localhash, 16) == 0)
		retval = 1;
	else
		retval = 0;
	close(fd);
	return retval;
}

int toy_user_register(const char *user_name, const char *pwdhash)
{
	u_int id;
	if(toy_user_get_id_by_name(user_name, &id) == 1)
		return 0;
	id = toy_user_global.users_num;
	++toy_user_global.users_num;

	u_int name_len = strlen(user_name);
	char *name = malloc(name_len + 1);
	strcpy(name, user_name);
	struct toy_node_s *new_node = malloc(sizeof(struct toy_node_s ));
	new_node->user_name = name;
	new_node->user_id = id;
	new_node->left = NULL;
	new_node->right = NULL;
	new_node->balance = 0;
	toy_index_add(&toy_user_global.users_index, new_node);

	int fd = open(toy_conf_global.option_userinfos_file, O_RDWR);
	if(fd == -1)
		toy_kill("open file error on toy_user_register()");
	lseek(fd, 0, SEEK_SET);
	write(fd, &toy_user_global.users_num, 4);
	lseek(fd, 0, SEEK_END);
	u_int name_size = name_len + 1;
	write(fd, &name_size, sizeof(u_int));
	write(fd, name, name_size);
	write(fd, &id, sizeof(u_int));
	close(fd);

	fd = open(toy_conf_global.option_pwdhashs_file, O_RDWR);
	if(fd == -1)
		toy_kill("open file error on toy_user_register()");
	lseek(fd, id * 16, SEEK_SET);
	write(fd, pwdhash, 16);
	close(fd);

	toy_vfile_create(id);

	return 1;
}