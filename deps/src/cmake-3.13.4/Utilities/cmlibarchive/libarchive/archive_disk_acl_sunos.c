/*-
 * Copyright (c) 2017 Martin Matuska
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "archive_platform.h"

#if ARCHIVE_ACL_SUNOS

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_ACL_H
#define _ACL_PRIVATE /* For debugging */
#include <sys/acl.h>
#endif

#include "archive_entry.h"
#include "archive_private.h"
#include "archive_read_disk_private.h"
#include "archive_write_disk_private.h"

typedef struct {
	const int a_perm;	/* Libarchive permission or flag */
	const int p_perm;	/* Platform permission or flag */
} acl_perm_map_t;

static const acl_perm_map_t acl_posix_perm_map[] = {
	{ARCHIVE_ENTRY_ACL_EXECUTE, S_IXOTH },
	{ARCHIVE_ENTRY_ACL_WRITE, S_IWOTH },
	{ARCHIVE_ENTRY_ACL_READ, S_IROTH }
};

static const int acl_posix_perm_map_size =
    (int)(sizeof(acl_posix_perm_map)/sizeof(acl_posix_perm_map[0]));

#if ARCHIVE_ACL_SUNOS_NFS4
static const acl_perm_map_t acl_nfs4_perm_map[] = {
	{ARCHIVE_ENTRY_ACL_EXECUTE, ACE_EXECUTE},
	{ARCHIVE_ENTRY_ACL_READ_DATA, ACE_READ_DATA},
	{ARCHIVE_ENTRY_ACL_LIST_DIRECTORY, ACE_LIST_DIRECTORY},
	{ARCHIVE_ENTRY_ACL_WRITE_DATA, ACE_WRITE_DATA},
	{ARCHIVE_ENTRY_ACL_ADD_FILE, ACE_ADD_FILE},
	{ARCHIVE_ENTRY_ACL_APPEND_DATA, ACE_APPEND_DATA},
	{ARCHIVE_ENTRY_ACL_ADD_SUBDIRECTORY, ACE_ADD_SUBDIRECTORY},
	{ARCHIVE_ENTRY_ACL_READ_NAMED_ATTRS, ACE_READ_NAMED_ATTRS},
	{ARCHIVE_ENTRY_ACL_WRITE_NAMED_ATTRS, ACE_WRITE_NAMED_ATTRS},
	{ARCHIVE_ENTRY_ACL_DELETE_CHILD, ACE_DELETE_CHILD},
	{ARCHIVE_ENTRY_ACL_READ_ATTRIBUTES, ACE_READ_ATTRIBUTES},
	{ARCHIVE_ENTRY_ACL_WRITE_ATTRIBUTES, ACE_WRITE_ATTRIBUTES},
	{ARCHIVE_ENTRY_ACL_DELETE, ACE_DELETE},
	{ARCHIVE_ENTRY_ACL_READ_ACL, ACE_READ_ACL},
	{ARCHIVE_ENTRY_ACL_WRITE_ACL, ACE_WRITE_ACL},
	{ARCHIVE_ENTRY_ACL_WRITE_OWNER, ACE_WRITE_OWNER},
	{ARCHIVE_ENTRY_ACL_SYNCHRONIZE, ACE_SYNCHRONIZE}
};

static const int acl_nfs4_perm_map_size =
    (int)(sizeof(acl_nfs4_perm_map)/sizeof(acl_nfs4_perm_map[0]));

static const acl_perm_map_t acl_nfs4_flag_map[] = {
	{ARCHIVE_ENTRY_ACL_ENTRY_FILE_INHERIT, ACE_FILE_INHERIT_ACE},
	{ARCHIVE_ENTRY_ACL_ENTRY_DIRECTORY_INHERIT, ACE_DIRECTORY_INHERIT_ACE},
	{ARCHIVE_ENTRY_ACL_ENTRY_NO_PROPAGATE_INHERIT, ACE_NO_PROPAGATE_INHERIT_ACE},
	{ARCHIVE_ENTRY_ACL_ENTRY_INHERIT_ONLY, ACE_INHERIT_ONLY_ACE},
	{ARCHIVE_ENTRY_ACL_ENTRY_SUCCESSFUL_ACCESS, ACE_SUCCESSFUL_ACCESS_ACE_FLAG},
	{ARCHIVE_ENTRY_ACL_ENTRY_FAILED_ACCESS, ACE_FAILED_ACCESS_ACE_FLAG},
#ifdef ACE_INHERITED_ACE
	{ARCHIVE_ENTRY_ACL_ENTRY_INHERITED, ACE_INHERITED_ACE}
#endif
};

const int acl_nfs4_flag_map_size =
    (int)(sizeof(acl_nfs4_flag_map)/sizeof(acl_nfs4_flag_map[0]));

#endif /* ARCHIVE_ACL_SUNOS_NFS4 */

static void *
sunacl_get(int cmd, int *aclcnt, int fd, const char *path)
{
	int cnt, cntcmd;
	size_t size;
	void *aclp;

	if (cmd == GETACL) {
		cntcmd = GETACLCNT;
		size = sizeof(aclent_t);
	}
#if ARCHIVE_ACL_SUNOS_NFS4
	else if (cmd == ACE_GETACL) {
		cntcmd = ACE_GETACLCNT;
		size = sizeof(ace_t);
	}
#endif
	else {
		errno = EINVAL;
		*aclcnt = -1;
		return (NULL);
	}

	aclp = NULL;
	cnt = -2;

	while (cnt == -2 || (cnt == -1 && errno == ENOSPC)) {
		if (path != NULL)
			cnt = acl(path, cntcmd, 0, NULL);
		else
			cnt = facl(fd, cntcmd, 0, NULL);

		if (cnt > 0) {
			if (aclp == NULL)
				aclp = malloc(cnt * size);
			else
				aclp = realloc(NULL, cnt * size);
			if (aclp != NULL) {
				if (path != NULL)
					cnt = acl(path, cmd, cnt, aclp);
				else
					cnt = facl(fd, cmd, cnt, aclp);
			}
		} else {
			if (aclp != NULL) {
				free(aclp);
				aclp = NULL;
			}
			break;
		}
	}

	*aclcnt = cnt;
	return (aclp);
}

/*
 * Check if acl is trivial
 * This is a FreeBSD acl_is_trivial_np() implementation for Solaris
 */
static int
sun_acl_is_trivial(void *aclp, int aclcnt, mode_t mode, int is_nfs4,
    int is_dir, int *trivialp)
{
#if ARCHIVE_ACL_SUNOS_NFS4
	int i, p;
	const uint32_t rperm = ACE_READ_DATA;
	const uint32_t wperm = ACE_WRITE_DATA | ACE_APPEND_DATA;
	const uint32_t eperm = ACE_EXECUTE;
	const uint32_t pubset = ACE_READ_ATTRIBUTES | ACE_READ_NAMED_ATTRS |
	    ACE_READ_ACL | ACE_SYNCHRONIZE;
	const uint32_t ownset = pubset | ACE_WRITE_ATTRIBUTES |
	    ACE_WRITE_NAMED_ATTRS | ACE_WRITE_ACL | ACE_WRITE_OWNER;

	ace_t *ace;
	ace_t tace[6];
#endif

	if (aclp == NULL || trivialp == NULL)
		return (-1);

	*trivialp = 0;

	/*
	 * POSIX.1e ACLs marked with ACL_IS_TRIVIAL are compatible with
	 * FreeBSD acl_is_trivial_np(). On Solaris they have 4 entries,
	 * including mask.
	 */
	if (!is_nfs4) {
		if (aclcnt == 4)
			*trivialp = 1;
		return (0);
	}

#if ARCHIVE_ACL_SUNOS_NFS4
	/*
	 * Continue with checking NFSv4 ACLs
	 *
	 * Create list of trivial ace's to be compared
	 */

	/* owner@ allow pre */
	tace[0].a_flags = ACE_OWNER;
	tace[0].a_type = ACE_ACCESS_ALLOWED_ACE_TYPE;
	tace[0].a_access_mask = 0;

	/* owner@ deny */
	tace[1].a_flags = ACE_OWNER;
	tace[1].a_type = ACE_ACCESS_DENIED_ACE_TYPE;
	tace[1].a_access_mask = 0;

	/* group@ deny */
	tace[2].a_flags = ACE_GROUP | ACE_IDENTIFIER_GROUP;
	tace[2].a_type = ACE_ACCESS_DENIED_ACE_TYPE;
	tace[2].a_access_mask = 0;

	/* owner@ allow */
	tace[3].a_flags = ACE_OWNER;
	tace[3].a_type = ACE_ACCESS_ALLOWED_ACE_TYPE;
	tace[3].a_access_mask = ownset;

	/* group@ allow */
	tace[4].a_flags = ACE_GROUP | ACE_IDENTIFIER_GROUP;
	tace[4].a_type = ACE_ACCESS_ALLOWED_ACE_TYPE;
	tace[4].a_access_mask = pubset;

	/* everyone@ allow */
	tace[5].a_flags = ACE_EVERYONE;
	tace[5].a_type = ACE_ACCESS_ALLOWED_ACE_TYPE;
	tace[5].a_access_mask = pubset;

	/* Permissions for everyone@ */
	if (mode & 0004)
		tace[5].a_access_mask |= rperm;
	if (mode & 0002)
		tace[5].a_access_mask |= wperm;
	if (mode & 0001)
		tace[5].a_access_mask |= eperm;

	/* Permissions for group@ */
	if (mode & 0040)
		tace[4].a_access_mask |= rperm;
	else if (mode & 0004)
		tace[2].a_access_mask |= rperm;
	if (mode & 0020)
		tace[4].a_access_mask |= wperm;
	else if (mode & 0002)
		tace[2].a_access_mask |= wperm;
	if (mode & 0010)
		tace[4].a_access_mask |= eperm;
	else if (mode & 0001)
		tace[2].a_access_mask |= eperm;

	/* Permissions for owner@ */
	if (mode & 0400) {
		tace[3].a_access_mask |= rperm;
		if (!(mode & 0040) && (mode & 0004))
			tace[0].a_access_mask |= rperm;
	} else if ((mode & 0040) || (mode & 0004))
		tace[1].a_access_mask |= rperm;
	if (mode & 0200) {
		tace[3].a_access_mask |= wperm;
		if (!(mode & 0020) && (mode & 0002))
			tace[0].a_access_mask |= wperm;
	} else if ((mode & 0020) || (mode & 0002))
		tace[1].a_access_mask |= wperm;
	if (mode & 0100) {
		tace[3].a_access_mask |= eperm;
		if (!(mode & 0010) && (mode & 0001))
			tace[0].a_access_mask |= eperm;
	} else if ((mode & 0010) || (mode & 0001))
		tace[1].a_access_mask |= eperm;

	/* Check if the acl count matches */
	p = 3;
	for (i = 0; i < 3; i++) {
		if (tace[i].a_access_mask != 0)
			p++;
	}
	if (aclcnt != p)
		return (0);

	p = 0;
	for (i = 0; i < 6; i++) {
		if (tace[i].a_access_mask != 0) {
			ace = &((ace_t *)aclp)[p];
			/*
			 * Illumos added ACE_DELETE_CHILD to write perms for
			 * directories. We have to check against that, too.
			 */
			if (ace->a_flags != tace[i].a_flags ||
			    ace->a_type != tace[i].a_type ||
			    (ace->a_access_mask != tace[i].a_access_mask &&
			    (!is_dir || (tace[i].a_access_mask & wperm) == 0 ||
			    ace->a_access_mask !=
			    (tace[i].a_access_mask | ACE_DELETE_CHILD))))
				return (0);
			p++;
		}
	}

	*trivialp = 1;
#else	/* !ARCHIVE_ACL_SUNOS_NFS4 */
	(void)is_dir;	/* UNUSED */
	(void)aclp;	/* UNUSED */
#endif	/* !ARCHIVE_ACL_SUNOS_NFS4 */
	return (0);
}

/*
 * Translate Solaris POSIX.1e and NFSv4 ACLs into libarchive internal ACL
 */
static int
translate_acl(struct archive_read_disk *a,
    struct archive_entry *entry, void *aclp, int aclcnt,
    int default_entry_acl_type)
{
	int e, i;
	int ae_id, ae_tag, ae_perm;
	int entry_acl_type;
	const char *ae_name;
	aclent_t *aclent;
#if ARCHIVE_ACL_SUNOS_NFS4
	ace_t *ace;
#endif

	if (aclcnt <= 0)
		return (ARCHIVE_OK);

	for (e = 0; e < aclcnt; e++) {
		ae_name = NULL;
		ae_tag = 0;
		ae_perm = 0;

#if ARCHIVE_ACL_SUNOS_NFS4
		if (default_entry_acl_type == ARCHIVE_ENTRY_ACL_TYPE_NFS4) {
			ace = &((ace_t *)aclp)[e];
			ae_id = ace->a_who;

			switch(ace->a_type) {
			case ACE_ACCESS_ALLOWED_ACE_TYPE:
				entry_acl_type = ARCHIVE_ENTRY_ACL_TYPE_ALLOW;
				break;
			case ACE_ACCESS_DENIED_ACE_TYPE:
				entry_acl_type = ARCHIVE_ENTRY_ACL_TYPE_DENY;
				break;
			case ACE_SYSTEM_AUDIT_ACE_TYPE:
				entry_acl_type = ARCHIVE_ENTRY_ACL_TYPE_ACCESS;
				break;
			case ACE_SYSTEM_ALARM_ACE_TYPE:
				entry_acl_type = ARCHIVE_ENTRY_ACL_TYPE_ALARM;
				break;
			default:
				/* Unknown entry type, skip */
				continue;
			}

			if ((ace->a_flags & ACE_OWNER) != 0)
				ae_tag = ARCHIVE_ENTRY_ACL_USER_OBJ;
			else if ((ace->a_flags & ACE_GROUP) != 0)
				ae_tag = ARCHIVE_ENTRY_ACL_GROUP_OBJ;
			else if ((ace->a_flags & ACE_EVERYONE) != 0)
				ae_tag = ARCHIVE_ENTRY_ACL_EVERYONE;
			else if ((ace->a_flags & ACE_IDENTIFIER_GROUP) != 0) {
				ae_tag = ARCHIVE_ENTRY_ACL_GROUP;
				ae_name = archive_read_disk_gname(&a->archive,
				    ae_id);
			} else {
				ae_tag = ARCHIVE_ENTRY_ACL_USER;
				ae_name = archive_read_disk_uname(&a->archive,
				    ae_id);
			}

			for (i = 0; i < acl_nfs4_flag_map_size; ++i) {
				if ((ace->a_flags &
				    acl_nfs4_flag_map[i].p_perm) != 0)
					ae_perm |= acl_nfs4_flag_map[i].a_perm;
			}

			for (i = 0; i < acl_nfs4_perm_map_size; ++i) {
				if ((ace->a_access_mask &
				    acl_nfs4_perm_map[i].p_perm) != 0)
					ae_perm |= acl_nfs4_perm_map[i].a_perm;
			}
		} else
#endif	/* ARCHIVE_ACL_SUNOS_NFS4 */
		if (default_entry_acl_type == ARCHIVE_ENTRY_ACL_TYPE_ACCESS) {
			aclent = &((aclent_t *)aclp)[e];
			if ((aclent->a_type & ACL_DEFAULT) != 0)
				entry_acl_type = ARCHIVE_ENTRY_ACL_TYPE_DEFAULT;
			else
				entry_acl_type = ARCHIVE_ENTRY_ACL_TYPE_ACCESS;
			ae_id = aclent->a_id;

			switch(aclent->a_type) {
			case DEF_USER:
			case USER:
				ae_name = archive_read_disk_uname(&a->archive,
				    ae_id);
				ae_tag = ARCHIVE_ENTRY_ACL_USER;
				break;
			case DEF_GROUP:
			case GROUP:
				ae_name = archive_read_disk_gname(&a->archive,
				    ae_id);
				ae_tag = ARCHIVE_ENTRY_ACL_GROUP;
				break;
			case DEF_CLASS_OBJ:
			case CLASS_OBJ:
				ae_tag = ARCHIVE_ENTRY_ACL_MASK;
				break;
			case DEF_USER_OBJ:
			case USER_OBJ:
				ae_tag = ARCHIVE_ENTRY_ACL_USER_OBJ;
				break;
			case DEF_GROUP_OBJ:
			case GROUP_OBJ:
				ae_tag = ARCHIVE_ENTRY_ACL_GROUP_OBJ;
				break;
			case DEF_OTHER_OBJ:
			case OTHER_OBJ:
				ae_tag = ARCHIVE_ENTRY_ACL_OTHER;
				break;
			default:
				/* Unknown tag type, skip */
				continue;
			}

			for (i = 0; i < acl_posix_perm_map_size; ++i) {
				if ((aclent->a_perm &
				    acl_posix_perm_map[i].p_perm) != 0)
					ae_perm |= acl_posix_perm_map[i].a_perm;
			}
		} else
			return (ARCHIVE_WARN);

		archive_entry_acl_add_entry(entry, entry_acl_type,
		    ae_perm, ae_tag, ae_id, ae_name);
	}
	return (ARCHIVE_OK);
}

static int
set_acl(struct archive *a, int fd, const char *name,
    struct archive_acl *abstract_acl,
    int ae_requested_type, const char *tname)
{
	aclent_t	 *aclent;
#if ARCHIVE_ACL_SUNOS_NFS4
	ace_t		 *ace;
#endif
	int		 cmd, e, r;
	void		 *aclp;
	int		 ret;
	int		 ae_type, ae_permset, ae_tag, ae_id;
	int		 perm_map_size;
	const acl_perm_map_t	*perm_map;
	uid_t		 ae_uid;
	gid_t		 ae_gid;
	const char	*ae_name;
	int		 entries;
	int		 i;

	ret = ARCHIVE_OK;
	entries = archive_acl_reset(abstract_acl, ae_requested_type);
	if (entries == 0)
		return (ARCHIVE_OK);


	switch (ae_requested_type) {
	case ARCHIVE_ENTRY_ACL_TYPE_POSIX1E:
		cmd = SETACL;
		aclp = malloc(entries * sizeof(aclent_t));
		break;
#if ARCHIVE_ACL_SUNOS_NFS4
	case ARCHIVE_ENTRY_ACL_TYPE_NFS4:
		cmd = ACE_SETACL;
		aclp = malloc(entries * sizeof(ace_t));

		break;
#endif
	default:
		errno = ENOENT;
		archive_set_error(a, errno, "Unsupported ACL type");
		return (ARCHIVE_FAILED);
	}

	if (aclp == NULL) {
		archive_set_error(a, errno,
		    "Can't allocate memory for acl buffer");
		return (ARCHIVE_FAILED);
	}

	e = 0;

	while (archive_acl_next(a, abstract_acl, ae_requested_type, &ae_type,
		   &ae_permset, &ae_tag, &ae_id, &ae_name) == ARCHIVE_OK) {
		aclent = NULL;
#if ARCHIVE_ACL_SUNOS_NFS4
		ace = NULL;
#endif
		if (cmd == SETACL) {
			aclent = &((aclent_t *)aclp)[e];
			aclent->a_id = -1;
			aclent->a_type = 0;
			aclent->a_perm = 0;
		}
#if ARCHIVE_ACL_SUNOS_NFS4
		else {	/* cmd == ACE_SETACL */
			ace = &((ace_t *)aclp)[e];
			ace->a_who = -1;
			ace->a_access_mask = 0;
			ace->a_flags = 0;
		}
#endif	/* ARCHIVE_ACL_SUNOS_NFS4 */

		switch (ae_tag) {
		case ARCHIVE_ENTRY_ACL_USER:
			ae_uid = archive_write_disk_uid(a, ae_name, ae_id);
			if (aclent != NULL) {
				aclent->a_id = ae_uid;
				aclent->a_type |= USER;
			}
#if ARCHIVE_ACL_SUNOS_NFS4
			else {
				ace->a_who = ae_uid;
			}
#endif
			break;
		case ARCHIVE_ENTRY_ACL_GROUP:
			ae_gid = archive_write_disk_gid(a, ae_name, ae_id);
			if (aclent != NULL) {
				aclent->a_id = ae_gid;
				aclent->a_type |= GROUP;
			}
#if ARCHIVE_ACL_SUNOS_NFS4
			else {
				ace->a_who = ae_gid;
				ace->a_flags |= ACE_IDENTIFIER_GROUP;
			}
#endif
			break;
		case ARCHIVE_ENTRY_ACL_USER_OBJ:
			if (aclent != NULL)
				aclent->a_type |= USER_OBJ;
#if ARCHIVE_ACL_SUNOS_NFS4
			else {
				ace->a_flags |= ACE_OWNER;
			}
#endif
			break;
		case ARCHIVE_ENTRY_ACL_GROUP_OBJ:
			if (aclent != NULL)
				aclent->a_type |= GROUP_OBJ;
#if ARCHIVE_ACL_SUNOS_NFS4
			else {
				ace->a_flags |= ACE_GROUP;
				ace->a_flags |= ACE_IDENTIFIER_GROUP;
			}
#endif
			break;
		case ARCHIVE_ENTRY_ACL_MASK:
			if (aclent != NULL)
				aclent->a_type |= CLASS_OBJ;
			break;
		case ARCHIVE_ENTRY_ACL_OTHER:
			if (aclent != NULL)
				aclent->a_type |= OTHER_OBJ;
			break;
#if ARCHIVE_ACL_SUNOS_NFS4
		case ARCHIVE_ENTRY_ACL_EVERYONE:
			if (ace != NULL)
				ace->a_flags |= ACE_EVERYONE;
			break;
#endif
		default:
			archive_set_error(a, ARCHIVE_ERRNO_MISC,
			    "Unsupported ACL tag");
			ret = ARCHIVE_FAILED;
			goto exit_free;
		}

		r = 0;
		switch (ae_type) {
#if ARCHIVE_ACL_SUNOS_NFS4
		case ARCHIVE_ENTRY_ACL_TYPE_ALLOW:
			if (ace != NULL)
				ace->a_type = ACE_ACCESS_ALLOWED_ACE_TYPE;
			else
				r = -1;
			break;
		case ARCHIVE_ENTRY_ACL_TYPE_DENY:
			if (ace != NULL)
				ace->a_type = ACE_ACCESS_DENIED_ACE_TYPE;
			else
				r = -1;
			break;
		case ARCHIVE_ENTRY_ACL_TYPE_AUDIT:
			if (ace != NULL)
				ace->a_type = ACE_SYSTEM_AUDIT_ACE_TYPE;
			else
				r = -1;
			break;
		case ARCHIVE_ENTRY_ACL_TYPE_ALARM:
			if (ace != NULL)
				ace->a_type = ACE_SYSTEM_ALARM_ACE_TYPE;
			else
				r = -1;
			break;
#endif
		case ARCHIVE_ENTRY_ACL_TYPE_ACCESS:
			if (aclent == NULL)
				r = -1;
			break;
		case ARCHIVE_ENTRY_ACL_TYPE_DEFAULT:
			if (aclent != NULL)
				aclent->a_type |= ACL_DEFAULT;
			else
				r = -1;
			break;
		default:
			archive_set_error(a, ARCHIVE_ERRNO_MISC,
			    "Unsupported ACL entry type");
			ret = ARCHIVE_FAILED;
			goto exit_free;
		}

		if (r != 0) {
			errno = EINVAL;
			archive_set_error(a, errno,
			    "Failed to set ACL entry type");
			ret = ARCHIVE_FAILED;
			goto exit_free;
		}

#if ARCHIVE_ACL_SUNOS_NFS4
		if (ae_requested_type == ARCHIVE_ENTRY_ACL_TYPE_NFS4) {
			perm_map_size = acl_nfs4_perm_map_size;
			perm_map = acl_nfs4_perm_map;
		} else {
#endif
			perm_map_size = acl_posix_perm_map_size;
			perm_map = acl_posix_perm_map;
#if ARCHIVE_ACL_SUNOS_NFS4
		}
#endif
		for (i = 0; i < perm_map_size; ++i) {
			if (ae_permset & perm_map[i].a_perm) {
#if ARCHIVE_ACL_SUNOS_NFS4
				if (ae_requested_type ==
				    ARCHIVE_ENTRY_ACL_TYPE_NFS4)
					ace->a_access_mask |=
					    perm_map[i].p_perm;
				else
#endif
					aclent->a_perm |= perm_map[i].p_perm;
			}
		}

#if ARCHIVE_ACL_SUNOS_NFS4
		if (ae_requested_type == ARCHIVE_ENTRY_ACL_TYPE_NFS4) {
			for (i = 0; i < acl_nfs4_flag_map_size; ++i) {
				if (ae_permset & acl_nfs4_flag_map[i].a_perm) {
					ace->a_flags |=
					    acl_nfs4_flag_map[i].p_perm;
				}
			}
		}
#endif
	e++;
	}

	/* Try restoring the ACL through 'fd' if we can. */
	if (fd >= 0) {
		if (facl(fd, cmd, entries, aclp) == 0)
			ret = ARCHIVE_OK;
		else {
			if (errno == EOPNOTSUPP) {
				/* Filesystem doesn't support ACLs */
				ret = ARCHIVE_OK;
			} else {
				archive_set_error(a, errno,
				    "Failed to set acl on fd: %s", tname);
				ret = ARCHIVE_WARN;
			}
		}
	} else if (acl(name, cmd, entries, aclp) != 0) {
		if (errno == EOPNOTSUPP) {
			/* Filesystem doesn't support ACLs */
			ret = ARCHIVE_OK;
		} else {
			archive_set_error(a, errno, "Failed to set acl: %s",
			    tname);
			ret = ARCHIVE_WARN;
		}
	}
exit_free:
	free(aclp);
	return (ret);
}

int
archive_read_disk_entry_setup_acls(struct archive_read_disk *a,
    struct archive_entry *entry, int *fd)
{
	const char	*accpath;
	void		*aclp;
	int		aclcnt;
	int		r;

	accpath = NULL;

	if (*fd < 0) {
		accpath = archive_read_disk_entry_setup_path(a, entry, fd);
		if (accpath == NULL)
			return (ARCHIVE_WARN);
	}

	archive_entry_acl_clear(entry);

	aclp = NULL;

#if ARCHIVE_ACL_SUNOS_NFS4
	if (*fd >= 0)
		aclp = sunacl_get(ACE_GETACL, &aclcnt, *fd, NULL);
	else if ((!a->follow_symlinks)
	    && (archive_entry_filetype(entry) == AE_IFLNK))
		/* We can't get the ACL of a symlink, so we assume it can't
		   have one. */
		aclp = NULL;
	else
		aclp = sunacl_get(ACE_GETACL, &aclcnt, 0, accpath);

	if (aclp != NULL && sun_acl_is_trivial(aclp, aclcnt,
	    archive_entry_mode(entry), 1, S_ISDIR(archive_entry_mode(entry)),
	    &r) == 0 && r == 1) {
		free(aclp);
		aclp = NULL;
		return (ARCHIVE_OK);
	}

	if (aclp != NULL) {
		r = translate_acl(a, entry, aclp, aclcnt,
		    ARCHIVE_ENTRY_ACL_TYPE_NFS4);
		free(aclp);
		aclp = NULL;

		if (r != ARCHIVE_OK) {
			archive_set_error(&a->archive, errno,
			    "Couldn't translate NFSv4 ACLs");
		}
		return (r);
	}
#endif	/* ARCHIVE_ACL_SUNOS_NFS4 */

	/* Retrieve POSIX.1e ACLs from file. */
	if (*fd >= 0)
		aclp = sunacl_get(GETACL, &aclcnt, *fd, NULL);
	else if ((!a->follow_symlinks)
	    && (archive_entry_filetype(entry) == AE_IFLNK))
		/* We can't get the ACL of a symlink, so we assume it can't
		   have one. */
		aclp = NULL;
	else
		aclp = sunacl_get(GETACL, &aclcnt, 0, accpath);

	/* Ignore "trivial" ACLs that just mirror the file mode. */
	if (aclp != NULL && sun_acl_is_trivial(aclp, aclcnt,
	    archive_entry_mode(entry), 0, S_ISDIR(archive_entry_mode(entry)),
	    &r) == 0 && r == 1) {
		free(aclp);
		aclp = NULL;
	}

	if (aclp != NULL)
	{
		r = translate_acl(a, entry, aclp, aclcnt,
		    ARCHIVE_ENTRY_ACL_TYPE_ACCESS);
		free(aclp);
		aclp = NULL;

		if (r != ARCHIVE_OK) {
			archive_set_error(&a->archive, errno,
			    "Couldn't translate access ACLs");
			return (r);
		}
	}

	return (ARCHIVE_OK);
}

int
archive_write_disk_set_acls(struct archive *a, int fd, const char *name,
    struct archive_acl *abstract_acl, __LA_MODE_T mode)
{
	int		ret = ARCHIVE_OK;

	(void)mode;	/* UNUSED */

	if ((archive_acl_types(abstract_acl)
	    & ARCHIVE_ENTRY_ACL_TYPE_POSIX1E) != 0) {
		/* Solaris writes POSIX.1e access and default ACLs together */
		ret = set_acl(a, fd, name, abstract_acl,
		    ARCHIVE_ENTRY_ACL_TYPE_POSIX1E, "posix1e");

		/* Simultaneous POSIX.1e and NFSv4 is not supported */
		return (ret);
	}
#if ARCHIVE_ACL_SUNOS_NFS4
	else if ((archive_acl_types(abstract_acl) &
	    ARCHIVE_ENTRY_ACL_TYPE_NFS4) != 0) {
		ret = set_acl(a, fd, name, abstract_acl,
		    ARCHIVE_ENTRY_ACL_TYPE_NFS4, "nfs4");
	}
#endif
	return (ret);
}
#endif	/* ARCHIVE_ACL_SUNOS */
