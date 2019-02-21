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

#if ARCHIVE_ACL_DARWIN

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_MEMBERSHIP_H
#include <membership.h>
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

static const acl_perm_map_t acl_nfs4_perm_map[] = {
	{ARCHIVE_ENTRY_ACL_READ_DATA, ACL_READ_DATA},
	{ARCHIVE_ENTRY_ACL_LIST_DIRECTORY, ACL_LIST_DIRECTORY},
	{ARCHIVE_ENTRY_ACL_WRITE_DATA, ACL_WRITE_DATA},
	{ARCHIVE_ENTRY_ACL_ADD_FILE, ACL_ADD_FILE},
	{ARCHIVE_ENTRY_ACL_EXECUTE, ACL_EXECUTE},
	{ARCHIVE_ENTRY_ACL_DELETE, ACL_DELETE},
	{ARCHIVE_ENTRY_ACL_APPEND_DATA, ACL_APPEND_DATA},
	{ARCHIVE_ENTRY_ACL_ADD_SUBDIRECTORY, ACL_ADD_SUBDIRECTORY},
	{ARCHIVE_ENTRY_ACL_DELETE_CHILD, ACL_DELETE_CHILD},
	{ARCHIVE_ENTRY_ACL_READ_ATTRIBUTES, ACL_READ_ATTRIBUTES},
	{ARCHIVE_ENTRY_ACL_WRITE_ATTRIBUTES, ACL_WRITE_ATTRIBUTES},
	{ARCHIVE_ENTRY_ACL_READ_NAMED_ATTRS, ACL_READ_EXTATTRIBUTES},
	{ARCHIVE_ENTRY_ACL_WRITE_NAMED_ATTRS, ACL_WRITE_EXTATTRIBUTES},
	{ARCHIVE_ENTRY_ACL_READ_ACL, ACL_READ_SECURITY},
	{ARCHIVE_ENTRY_ACL_WRITE_ACL, ACL_WRITE_SECURITY},
	{ARCHIVE_ENTRY_ACL_WRITE_OWNER, ACL_CHANGE_OWNER},
#if HAVE_DECL_ACL_SYNCHRONIZE
	{ARCHIVE_ENTRY_ACL_SYNCHRONIZE, ACL_SYNCHRONIZE}
#endif
};

static const int acl_nfs4_perm_map_size =
    (int)(sizeof(acl_nfs4_perm_map)/sizeof(acl_nfs4_perm_map[0]));

static const acl_perm_map_t acl_nfs4_flag_map[] = {
	{ARCHIVE_ENTRY_ACL_ENTRY_INHERITED, ACL_ENTRY_INHERITED},
	{ARCHIVE_ENTRY_ACL_ENTRY_FILE_INHERIT, ACL_ENTRY_FILE_INHERIT},
	{ARCHIVE_ENTRY_ACL_ENTRY_DIRECTORY_INHERIT, ACL_ENTRY_DIRECTORY_INHERIT},
	{ARCHIVE_ENTRY_ACL_ENTRY_NO_PROPAGATE_INHERIT, ACL_ENTRY_LIMIT_INHERIT},
	{ARCHIVE_ENTRY_ACL_ENTRY_INHERIT_ONLY, ACL_ENTRY_ONLY_INHERIT}
};

static const int acl_nfs4_flag_map_size =
    (int)(sizeof(acl_nfs4_flag_map)/sizeof(acl_nfs4_flag_map[0]));

static int translate_guid(struct archive *a, acl_entry_t acl_entry,
    int *ae_id, int *ae_tag, const char **ae_name)
{
	void *q;
	uid_t ugid;
	int r, idtype;

	q = acl_get_qualifier(acl_entry);
	if (q == NULL)
		return (1);
	r = mbr_uuid_to_id((const unsigned char *)q, &ugid, &idtype);
	if (r != 0) {
		acl_free(q);
		return (1);
	}
	if (idtype == ID_TYPE_UID) {
		*ae_tag = ARCHIVE_ENTRY_ACL_USER;
		*ae_id = ugid;
		*ae_name = archive_read_disk_uname(a, *ae_id);
	} else if (idtype == ID_TYPE_GID) {
		*ae_tag = ARCHIVE_ENTRY_ACL_GROUP;
		*ae_id = ugid;
		*ae_name = archive_read_disk_gname(a, *ae_id);
	} else
		r = 1;

	acl_free(q);
	return (r);
}

static void
add_trivial_nfs4_acl(struct archive_entry *entry)
{
	mode_t mode;
	int i;
	const int rperm = ARCHIVE_ENTRY_ACL_READ_DATA;
	const int wperm = ARCHIVE_ENTRY_ACL_WRITE_DATA |
	    ARCHIVE_ENTRY_ACL_APPEND_DATA;
	const int eperm = ARCHIVE_ENTRY_ACL_EXECUTE;
	const int pubset = ARCHIVE_ENTRY_ACL_READ_ATTRIBUTES |
	    ARCHIVE_ENTRY_ACL_READ_NAMED_ATTRS |
	    ARCHIVE_ENTRY_ACL_READ_ACL |
	    ARCHIVE_ENTRY_ACL_SYNCHRONIZE;
	const int ownset = pubset | ARCHIVE_ENTRY_ACL_WRITE_ATTRIBUTES |
	    ARCHIVE_ENTRY_ACL_WRITE_NAMED_ATTRS |
	    ARCHIVE_ENTRY_ACL_WRITE_ACL |
	    ARCHIVE_ENTRY_ACL_WRITE_OWNER;

	struct {
	    const int type;
	    const int tag;
	    int permset;
	} tacl_entry[] = {
	    {ARCHIVE_ENTRY_ACL_TYPE_ALLOW, ARCHIVE_ENTRY_ACL_USER_OBJ, 0},
	    {ARCHIVE_ENTRY_ACL_TYPE_DENY, ARCHIVE_ENTRY_ACL_USER_OBJ, 0},
	    {ARCHIVE_ENTRY_ACL_TYPE_DENY, ARCHIVE_ENTRY_ACL_GROUP_OBJ, 0},
	    {ARCHIVE_ENTRY_ACL_TYPE_ALLOW, ARCHIVE_ENTRY_ACL_USER_OBJ, ownset},
	    {ARCHIVE_ENTRY_ACL_TYPE_ALLOW, ARCHIVE_ENTRY_ACL_GROUP_OBJ, pubset},
	    {ARCHIVE_ENTRY_ACL_TYPE_ALLOW, ARCHIVE_ENTRY_ACL_EVERYONE, pubset}
	};

	mode = archive_entry_mode(entry);

	/* Permissions for everyone@ */
	if (mode & 0004)
		tacl_entry[5].permset |= rperm;
	if (mode & 0002)
		tacl_entry[5].permset |= wperm;
	if (mode & 0001)
		tacl_entry[5].permset |= eperm;

	/* Permissions for group@ */
	if (mode & 0040)
		tacl_entry[4].permset |= rperm;
	else if (mode & 0004)
		tacl_entry[2].permset |= rperm;
	if (mode & 0020)
		tacl_entry[4].permset |= wperm;
	else if (mode & 0002)
		tacl_entry[2].permset |= wperm;
	if (mode & 0010)
		tacl_entry[4].permset |= eperm;
	else if (mode & 0001)
		tacl_entry[2].permset |= eperm;

	/* Permissions for owner@ */
	if (mode & 0400) {
		tacl_entry[3].permset |= rperm;
		if (!(mode & 0040) && (mode & 0004))
			tacl_entry[0].permset |= rperm;
	} else if ((mode & 0040) || (mode & 0004))
		tacl_entry[1].permset |= rperm;
	if (mode & 0200) {
		tacl_entry[3].permset |= wperm;
		if (!(mode & 0020) && (mode & 0002))
			tacl_entry[0].permset |= wperm;
	} else if ((mode & 0020) || (mode & 0002))
		tacl_entry[1].permset |= wperm;
	if (mode & 0100) {
		tacl_entry[3].permset |= eperm;
		if (!(mode & 0010) && (mode & 0001))
			tacl_entry[0].permset |= eperm;
	} else if ((mode & 0010) || (mode & 0001))
		tacl_entry[1].permset |= eperm;

	for (i = 0; i < 6; i++) {
		if (tacl_entry[i].permset != 0) {
			archive_entry_acl_add_entry(entry,
			    tacl_entry[i].type, tacl_entry[i].permset,
			    tacl_entry[i].tag, -1, NULL);
		}
	}

	return;
}

static int
translate_acl(struct archive_read_disk *a,
    struct archive_entry *entry, acl_t acl)
{
	acl_tag_t	 acl_tag;
	acl_flagset_t	 acl_flagset;
	acl_entry_t	 acl_entry;
	acl_permset_t	 acl_permset;
	int		 i, entry_acl_type;
	int		 r, s, ae_id, ae_tag, ae_perm;
	const char	*ae_name;

	s = acl_get_entry(acl, ACL_FIRST_ENTRY, &acl_entry);
	if (s == -1) {
		archive_set_error(&a->archive, errno,
		    "Failed to get first ACL entry");
		return (ARCHIVE_WARN);
	}

	while (s == 0) {
		ae_id = -1;
		ae_name = NULL;
		ae_perm = 0;

		if (acl_get_tag_type(acl_entry, &acl_tag) != 0) {
			archive_set_error(&a->archive, errno,
			    "Failed to get ACL tag type");
			return (ARCHIVE_WARN);
		}
		switch (acl_tag) {
		case ACL_EXTENDED_ALLOW:
			entry_acl_type = ARCHIVE_ENTRY_ACL_TYPE_ALLOW;
			r = translate_guid(&a->archive, acl_entry,
			    &ae_id, &ae_tag, &ae_name);
			break;
		case ACL_EXTENDED_DENY:
			entry_acl_type = ARCHIVE_ENTRY_ACL_TYPE_DENY;
			r = translate_guid(&a->archive, acl_entry,
			    &ae_id, &ae_tag, &ae_name);
			break;
		default:
			/* Skip types that libarchive can't support. */
			s = acl_get_entry(acl, ACL_NEXT_ENTRY, &acl_entry);
			continue;
		}

		/* Skip if translate_guid() above failed */
		if (r != 0) {
			s = acl_get_entry(acl, ACL_NEXT_ENTRY, &acl_entry);
			continue;
		}

		/*
		 * Libarchive stores "flag" (NFSv4 inheritance bits)
		 * in the ae_perm bitmap.
		 *
		 * acl_get_flagset_np() fails with non-NFSv4 ACLs
		 */
		if (acl_get_flagset_np(acl_entry, &acl_flagset) != 0) {
			archive_set_error(&a->archive, errno,
			    "Failed to get flagset from a NFSv4 ACL entry");
			return (ARCHIVE_WARN);
		}
		for (i = 0; i < acl_nfs4_flag_map_size; ++i) {
			r = acl_get_flag_np(acl_flagset,
			    acl_nfs4_flag_map[i].p_perm);
			if (r == -1) {
				archive_set_error(&a->archive, errno,
				    "Failed to check flag in a NFSv4 "
				    "ACL flagset");
				return (ARCHIVE_WARN);
			} else if (r)
				ae_perm |= acl_nfs4_flag_map[i].a_perm;
		}

		if (acl_get_permset(acl_entry, &acl_permset) != 0) {
			archive_set_error(&a->archive, errno,
			    "Failed to get ACL permission set");
			return (ARCHIVE_WARN);
		}

		for (i = 0; i < acl_nfs4_perm_map_size; ++i) {
			/*
			 * acl_get_perm() is spelled differently on different
			 * platforms; see above.
			 */
			r = acl_get_perm_np(acl_permset,
			    acl_nfs4_perm_map[i].p_perm);
			if (r == -1) {
				archive_set_error(&a->archive, errno,
				    "Failed to check permission in an ACL "
				    "permission set");
				return (ARCHIVE_WARN);
			} else if (r)
				ae_perm |= acl_nfs4_perm_map[i].a_perm;
		}

#if !HAVE_DECL_ACL_SYNCHRONIZE
		/* On Mac OS X without ACL_SYNCHRONIZE assume it is set */
		ae_perm |= ARCHIVE_ENTRY_ACL_SYNCHRONIZE;
#endif

		archive_entry_acl_add_entry(entry, entry_acl_type,
					    ae_perm, ae_tag,
					    ae_id, ae_name);

		s = acl_get_entry(acl, ACL_NEXT_ENTRY, &acl_entry);
	}
	return (ARCHIVE_OK);
}

static int
set_acl(struct archive *a, int fd, const char *name,
    struct archive_acl *abstract_acl,
    int ae_requested_type, const char *tname)
{
	acl_t		 acl;
	acl_entry_t	 acl_entry;
	acl_permset_t	 acl_permset;
	acl_flagset_t	 acl_flagset;
	int		 ret;
	int		 ae_type, ae_permset, ae_tag, ae_id;
	uuid_t		 ae_uuid;
	uid_t		 ae_uid;
	gid_t		 ae_gid;
	const char	*ae_name;
	int		 entries;
	int		 i;

	ret = ARCHIVE_OK;
	entries = archive_acl_reset(abstract_acl, ae_requested_type);
	if (entries == 0)
		return (ARCHIVE_OK);

	if (ae_requested_type != ARCHIVE_ENTRY_ACL_TYPE_NFS4) {
		errno = ENOENT;
		archive_set_error(a, errno, "Unsupported ACL type");
		return (ARCHIVE_FAILED);
	}

	acl = acl_init(entries);
	if (acl == (acl_t)NULL) {
		archive_set_error(a, errno,
		    "Failed to initialize ACL working storage");
		return (ARCHIVE_FAILED);
	}

	while (archive_acl_next(a, abstract_acl, ae_requested_type, &ae_type,
		   &ae_permset, &ae_tag, &ae_id, &ae_name) == ARCHIVE_OK) {
		/*
		 * Mac OS doesn't support NFSv4 ACLs for
		 * owner@, group@ and everyone@.
		 * We skip any of these ACLs found.
		 */
		if (ae_tag == ARCHIVE_ENTRY_ACL_USER_OBJ ||
		    ae_tag == ARCHIVE_ENTRY_ACL_GROUP_OBJ ||
		    ae_tag == ARCHIVE_ENTRY_ACL_EVERYONE)
			continue;

		if (acl_create_entry(&acl, &acl_entry) != 0) {
			archive_set_error(a, errno,
			    "Failed to create a new ACL entry");
			ret = ARCHIVE_FAILED;
			goto exit_free;
		}

		switch (ae_type) {
		case ARCHIVE_ENTRY_ACL_TYPE_ALLOW:
			acl_set_tag_type(acl_entry, ACL_EXTENDED_ALLOW);
			break;
		case ARCHIVE_ENTRY_ACL_TYPE_DENY:
			acl_set_tag_type(acl_entry, ACL_EXTENDED_DENY);
			break;
		default:
			/* We don't support any other types on MacOS */
			continue;
		}

		switch (ae_tag) {
		case ARCHIVE_ENTRY_ACL_USER:
			ae_uid = archive_write_disk_uid(a, ae_name, ae_id);
			if (mbr_uid_to_uuid(ae_uid, ae_uuid) != 0)
				continue;
			if (acl_set_qualifier(acl_entry, &ae_uuid) != 0)
				continue;
			break;
		case ARCHIVE_ENTRY_ACL_GROUP:
			ae_gid = archive_write_disk_gid(a, ae_name, ae_id);
			if (mbr_gid_to_uuid(ae_gid, ae_uuid) != 0)
				continue;
			if (acl_set_qualifier(acl_entry, &ae_uuid) != 0)
				continue;
			break;
		default:
			archive_set_error(a, ARCHIVE_ERRNO_MISC,
			    "Unsupported ACL tag");
			ret = ARCHIVE_FAILED;
			goto exit_free;
		}

		if (acl_get_permset(acl_entry, &acl_permset) != 0) {
			archive_set_error(a, errno,
			    "Failed to get ACL permission set");
			ret = ARCHIVE_FAILED;
			goto exit_free;
		}
		if (acl_clear_perms(acl_permset) != 0) {
			archive_set_error(a, errno,
			    "Failed to clear ACL permissions");
			ret = ARCHIVE_FAILED;
			goto exit_free;
		}

		for (i = 0; i < acl_nfs4_perm_map_size; ++i) {
			if (ae_permset & acl_nfs4_perm_map[i].a_perm) {
				if (acl_add_perm(acl_permset,
				    acl_nfs4_perm_map[i].p_perm) != 0) {
					archive_set_error(a, errno,
					    "Failed to add ACL permission");
					ret = ARCHIVE_FAILED;
					goto exit_free;
				}
			}
		}

		/*
		 * acl_get_flagset_np() fails with non-NFSv4 ACLs
		 */
		if (acl_get_flagset_np(acl_entry, &acl_flagset) != 0) {
			archive_set_error(a, errno,
			    "Failed to get flagset from an NFSv4 ACL entry");
			ret = ARCHIVE_FAILED;
			goto exit_free;
		}
		if (acl_clear_flags_np(acl_flagset) != 0) {
			archive_set_error(a, errno,
			    "Failed to clear flags from an NFSv4 ACL flagset");
			ret = ARCHIVE_FAILED;
			goto exit_free;
		}

		for (i = 0; i < acl_nfs4_flag_map_size; ++i) {
			if (ae_permset & acl_nfs4_flag_map[i].a_perm) {
				if (acl_add_flag_np(acl_flagset,
				    acl_nfs4_flag_map[i].p_perm) != 0) {
					archive_set_error(a, errno,
					    "Failed to add flag to "
					    "NFSv4 ACL flagset");
					ret = ARCHIVE_FAILED;
					goto exit_free;
				}
			}
		}
	}

	if (fd >= 0) {
		if (acl_set_fd_np(fd, acl, ACL_TYPE_EXTENDED) == 0)
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
	} else if (acl_set_link_np(name, ACL_TYPE_EXTENDED, acl) != 0) {
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
	acl_free(acl);
	return (ret);
}

int
archive_read_disk_entry_setup_acls(struct archive_read_disk *a,
    struct archive_entry *entry, int *fd)
{
	const char	*accpath;
	acl_t		acl;
	int		r;

	accpath = NULL;

	if (*fd < 0) {
		accpath = archive_read_disk_entry_setup_path(a, entry, fd);
		if (accpath == NULL)
			return (ARCHIVE_WARN);
	}

	archive_entry_acl_clear(entry);

	acl = NULL;

	if (*fd >= 0)
		acl = acl_get_fd_np(*fd, ACL_TYPE_EXTENDED);
	else if (!a->follow_symlinks)
		acl = acl_get_link_np(accpath, ACL_TYPE_EXTENDED);
	else
		acl = acl_get_file(accpath, ACL_TYPE_EXTENDED);

	if (acl != NULL) {
		r = translate_acl(a, entry, acl);
		acl_free(acl);
		acl = NULL;

		if (r != ARCHIVE_OK) {
			archive_set_error(&a->archive, errno,
			    "Couldn't translate NFSv4 ACLs");
		}

		/*
		 * Because Mac OS doesn't support owner@, group@ and everyone@
		 * ACLs we need to add NFSv4 ACLs mirroring the file mode to
		 * the archive entry. Otherwise extraction on non-Mac platforms
		 * would lead to an invalid file mode.
		 */
		if ((archive_entry_acl_types(entry) &
		    ARCHIVE_ENTRY_ACL_TYPE_NFS4) != 0)
			add_trivial_nfs4_acl(entry);

		return (r);
	}
	return (ARCHIVE_OK);
}

int
archive_write_disk_set_acls(struct archive *a, int fd, const char *name,
    struct archive_acl *abstract_acl, __LA_MODE_T mode)
{
	int		ret = ARCHIVE_OK;

	(void)mode;	/* UNUSED */

	if ((archive_acl_types(abstract_acl) &
	    ARCHIVE_ENTRY_ACL_TYPE_NFS4) != 0) {
		ret = set_acl(a, fd, name, abstract_acl,
		    ARCHIVE_ENTRY_ACL_TYPE_NFS4, "nfs4");
	}
	return (ret);
}
#endif	/* ARCHIVE_ACL_DARWIN */
