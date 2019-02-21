/*-
 * Copyright (c) 2003-2009 Tim Kientzle
 * Copyright (c) 2010-2012 Michihiro NAKAJIMA
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

#if ARCHIVE_ACL_LIBACL || ARCHIVE_ACL_LIBRICHACL

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_ACL_LIBACL_H
#include <acl/libacl.h>
#endif
#ifdef HAVE_SYS_ACL_H
#include <sys/acl.h>
#endif
#ifdef HAVE_SYS_RICHACL_H
#include <sys/richacl.h>
#endif

#include "archive_entry.h"
#include "archive_private.h"
#include "archive_read_disk_private.h"
#include "archive_write_disk_private.h"

typedef struct {
	const int a_perm;	/* Libarchive permission or flag */
	const int p_perm;	/* Platform permission or flag */
} acl_perm_map_t;

#if ARCHIVE_ACL_LIBACL
static const acl_perm_map_t acl_posix_perm_map[] = {
	{ARCHIVE_ENTRY_ACL_EXECUTE, ACL_EXECUTE},
	{ARCHIVE_ENTRY_ACL_WRITE, ACL_WRITE},
	{ARCHIVE_ENTRY_ACL_READ, ACL_READ},
};

static const int acl_posix_perm_map_size =
    (int)(sizeof(acl_posix_perm_map)/sizeof(acl_posix_perm_map[0]));
#endif /* ARCHIVE_ACL_LIBACL */

#if ARCHIVE_ACL_LIBRICHACL
static const acl_perm_map_t acl_nfs4_perm_map[] = {
	{ARCHIVE_ENTRY_ACL_EXECUTE, RICHACE_EXECUTE},
	{ARCHIVE_ENTRY_ACL_READ_DATA, RICHACE_READ_DATA},
	{ARCHIVE_ENTRY_ACL_LIST_DIRECTORY, RICHACE_LIST_DIRECTORY},
	{ARCHIVE_ENTRY_ACL_WRITE_DATA, RICHACE_WRITE_DATA},
	{ARCHIVE_ENTRY_ACL_ADD_FILE, RICHACE_ADD_FILE},
	{ARCHIVE_ENTRY_ACL_APPEND_DATA, RICHACE_APPEND_DATA},
	{ARCHIVE_ENTRY_ACL_ADD_SUBDIRECTORY, RICHACE_ADD_SUBDIRECTORY},
	{ARCHIVE_ENTRY_ACL_READ_NAMED_ATTRS, RICHACE_READ_NAMED_ATTRS},
	{ARCHIVE_ENTRY_ACL_WRITE_NAMED_ATTRS, RICHACE_WRITE_NAMED_ATTRS},
	{ARCHIVE_ENTRY_ACL_DELETE_CHILD, RICHACE_DELETE_CHILD},
	{ARCHIVE_ENTRY_ACL_READ_ATTRIBUTES, RICHACE_READ_ATTRIBUTES},
	{ARCHIVE_ENTRY_ACL_WRITE_ATTRIBUTES, RICHACE_WRITE_ATTRIBUTES},
	{ARCHIVE_ENTRY_ACL_DELETE, RICHACE_DELETE},
	{ARCHIVE_ENTRY_ACL_READ_ACL, RICHACE_READ_ACL},
	{ARCHIVE_ENTRY_ACL_WRITE_ACL, RICHACE_WRITE_ACL},
	{ARCHIVE_ENTRY_ACL_WRITE_OWNER, RICHACE_WRITE_OWNER},
	{ARCHIVE_ENTRY_ACL_SYNCHRONIZE, RICHACE_SYNCHRONIZE}
};

static const int acl_nfs4_perm_map_size =
    (int)(sizeof(acl_nfs4_perm_map)/sizeof(acl_nfs4_perm_map[0]));

static const acl_perm_map_t acl_nfs4_flag_map[] = {
	{ARCHIVE_ENTRY_ACL_ENTRY_FILE_INHERIT, RICHACE_FILE_INHERIT_ACE},
	{ARCHIVE_ENTRY_ACL_ENTRY_DIRECTORY_INHERIT, RICHACE_DIRECTORY_INHERIT_ACE},
	{ARCHIVE_ENTRY_ACL_ENTRY_NO_PROPAGATE_INHERIT, RICHACE_NO_PROPAGATE_INHERIT_ACE},
	{ARCHIVE_ENTRY_ACL_ENTRY_INHERIT_ONLY, RICHACE_INHERIT_ONLY_ACE},
	{ARCHIVE_ENTRY_ACL_ENTRY_INHERITED, RICHACE_INHERITED_ACE}
};

static const int acl_nfs4_flag_map_size =
    (int)(sizeof(acl_nfs4_flag_map)/sizeof(acl_nfs4_flag_map[0]));
#endif /* ARCHIVE_ACL_LIBRICHACL */

#if ARCHIVE_ACL_LIBACL
/*
 * Translate POSIX.1e ACLs into libarchive internal structure
 */
static int
translate_acl(struct archive_read_disk *a,
    struct archive_entry *entry, acl_t acl, int default_entry_acl_type)
{
	acl_tag_t	 acl_tag;
	acl_entry_t	 acl_entry;
	acl_permset_t	 acl_permset;
	int		 i, entry_acl_type;
	int		 r, s, ae_id, ae_tag, ae_perm;
	void		*q;
	const char	*ae_name;

	s = acl_get_entry(acl, ACL_FIRST_ENTRY, &acl_entry);
	if (s == -1) {
		archive_set_error(&a->archive, errno,
		    "Failed to get first ACL entry");
		return (ARCHIVE_WARN);
	}

	while (s == 1) {
		ae_id = -1;
		ae_name = NULL;
		ae_perm = 0;

		if (acl_get_tag_type(acl_entry, &acl_tag) != 0) {
			archive_set_error(&a->archive, errno,
			    "Failed to get ACL tag type");
			return (ARCHIVE_WARN);
		}
		switch (acl_tag) {
		case ACL_USER:
			q = acl_get_qualifier(acl_entry);
			if (q != NULL) {
				ae_id = (int)*(uid_t *)q;
				acl_free(q);
				ae_name = archive_read_disk_uname(&a->archive,
				    ae_id);
			}
			ae_tag = ARCHIVE_ENTRY_ACL_USER;
			break;
		case ACL_GROUP:
			q = acl_get_qualifier(acl_entry);
			if (q != NULL) {
				ae_id = (int)*(gid_t *)q;
				acl_free(q);
				ae_name = archive_read_disk_gname(&a->archive,
				    ae_id);
			}
			ae_tag = ARCHIVE_ENTRY_ACL_GROUP;
			break;
		case ACL_MASK:
			ae_tag = ARCHIVE_ENTRY_ACL_MASK;
			break;
		case ACL_USER_OBJ:
			ae_tag = ARCHIVE_ENTRY_ACL_USER_OBJ;
			break;
		case ACL_GROUP_OBJ:
			ae_tag = ARCHIVE_ENTRY_ACL_GROUP_OBJ;
			break;
		case ACL_OTHER:
			ae_tag = ARCHIVE_ENTRY_ACL_OTHER;
			break;
		default:
			/* Skip types that libarchive can't support. */
			s = acl_get_entry(acl, ACL_NEXT_ENTRY, &acl_entry);
			continue;
		}

		// XXX acl_type maps to allow/deny/audit/YYYY bits
		entry_acl_type = default_entry_acl_type;

		if (acl_get_permset(acl_entry, &acl_permset) != 0) {
			archive_set_error(&a->archive, errno,
			    "Failed to get ACL permission set");
			return (ARCHIVE_WARN);
		}

		for (i = 0; i < acl_posix_perm_map_size; ++i) {
			r = acl_get_perm(acl_permset,
			    acl_posix_perm_map[i].p_perm);
			if (r == -1) {
				archive_set_error(&a->archive, errno,
				    "Failed to check permission in an ACL "
				    "permission set");
				return (ARCHIVE_WARN);
			} else if (r)
				ae_perm |= acl_posix_perm_map[i].a_perm;
		}

		archive_entry_acl_add_entry(entry, entry_acl_type,
					    ae_perm, ae_tag,
					    ae_id, ae_name);

		s = acl_get_entry(acl, ACL_NEXT_ENTRY, &acl_entry);
		if (s == -1) {
			archive_set_error(&a->archive, errno,
			    "Failed to get next ACL entry");
			return (ARCHIVE_WARN);
		}
	}
	return (ARCHIVE_OK);
}
#endif /* ARCHIVE_ACL_LIBACL */

#if ARCHIVE_ACL_LIBRICHACL
/*
 * Translate RichACL into libarchive internal ACL
 */
static int
translate_richacl(struct archive_read_disk *a, struct archive_entry *entry,
    struct richacl *richacl)
{
	int ae_id, ae_tag, ae_perm;
	int entry_acl_type, i;
	const char *ae_name;

	struct richace *richace;

	richacl_for_each_entry(richace, richacl) {
		ae_name = NULL;
		ae_tag = 0;
		ae_perm = 0;
		ae_id = -1;

		switch (richace->e_type) {
		case RICHACE_ACCESS_ALLOWED_ACE_TYPE:
			entry_acl_type = ARCHIVE_ENTRY_ACL_TYPE_ALLOW;
			break;
		case RICHACE_ACCESS_DENIED_ACE_TYPE:
			entry_acl_type = ARCHIVE_ENTRY_ACL_TYPE_DENY;
			break;
		default: /* Unknown entry type, skip */
			continue;
		}

		/* Unsupported */
		if (richace->e_flags & RICHACE_UNMAPPED_WHO)
			continue;

		if (richace->e_flags & RICHACE_SPECIAL_WHO) {
			switch (richace->e_id) {
			case RICHACE_OWNER_SPECIAL_ID:
				ae_tag = ARCHIVE_ENTRY_ACL_USER_OBJ;
				break;
			case RICHACE_GROUP_SPECIAL_ID:
				ae_tag = ARCHIVE_ENTRY_ACL_GROUP_OBJ;
				break;
			case RICHACE_EVERYONE_SPECIAL_ID:
				ae_tag = ARCHIVE_ENTRY_ACL_EVERYONE;
				break;
			default: /* Unknown special ID type */
				continue;
			}
		} else {
			ae_id = richace->e_id;
			if (richace->e_flags & RICHACE_IDENTIFIER_GROUP) {
				ae_tag = ARCHIVE_ENTRY_ACL_GROUP;
				ae_name = archive_read_disk_gname(&a->archive,
				    (gid_t)(richace->e_id));
			} else {
				ae_tag = ARCHIVE_ENTRY_ACL_USER;
				ae_name = archive_read_disk_uname(&a->archive,
				    (uid_t)(richace->e_id));
			}
		}
		for (i = 0; i < acl_nfs4_flag_map_size; ++i) {
			if ((richace->e_flags &
			    acl_nfs4_flag_map[i].p_perm) != 0)
				ae_perm |= acl_nfs4_flag_map[i].a_perm;
		}
		for (i = 0; i < acl_nfs4_perm_map_size; ++i) {
			if ((richace->e_mask &
			    acl_nfs4_perm_map[i].p_perm) != 0)
				ae_perm |=
				    acl_nfs4_perm_map[i].a_perm;
		}

		archive_entry_acl_add_entry(entry, entry_acl_type,
		    ae_perm, ae_tag, ae_id, ae_name);
	}
	return (ARCHIVE_OK);
}
#endif	/* ARCHIVE_ACL_LIBRICHACL */

#if ARCHIVE_ACL_LIBRICHACL
static int
_richacl_mode_to_mask(short mode)
{
	int mask = 0;

	if (mode & S_IROTH)
		mask |= RICHACE_POSIX_MODE_READ;
	if (mode & S_IWOTH)
		mask |= RICHACE_POSIX_MODE_WRITE;
	if (mode & S_IXOTH)
		mask |= RICHACE_POSIX_MODE_EXEC;

	return (mask);
}

static void
_richacl_mode_to_masks(struct richacl *richacl, __LA_MODE_T mode)
{
	richacl->a_owner_mask = _richacl_mode_to_mask((mode & 0700) >> 6);
	richacl->a_group_mask = _richacl_mode_to_mask((mode & 0070) >> 3);
	richacl->a_other_mask = _richacl_mode_to_mask(mode & 0007);
}
#endif /* ARCHIVE_ACL_LIBRICHACL */

#if ARCHIVE_ACL_LIBRICHACL
static int
set_richacl(struct archive *a, int fd, const char *name,
    struct archive_acl *abstract_acl, __LA_MODE_T mode,
    int ae_requested_type, const char *tname)
{
	int		 ae_type, ae_permset, ae_tag, ae_id;
	uid_t		 ae_uid;
	gid_t		 ae_gid;
	const char	*ae_name;
	int		 entries;
	int		 i;
	int		 ret;
	int		 e = 0;
	struct richacl  *richacl = NULL;
	struct richace  *richace;

	ret = ARCHIVE_OK;
	entries = archive_acl_reset(abstract_acl, ae_requested_type);
	if (entries == 0)
		return (ARCHIVE_OK);

	if (ae_requested_type != ARCHIVE_ENTRY_ACL_TYPE_NFS4) {
		errno = ENOENT;
		archive_set_error(a, errno, "Unsupported ACL type");
		return (ARCHIVE_FAILED);
	}

	richacl = richacl_alloc(entries);
	if (richacl == NULL) {
		archive_set_error(a, errno,
			"Failed to initialize RichACL working storage");
		return (ARCHIVE_FAILED);
	}

	e = 0;

	while (archive_acl_next(a, abstract_acl, ae_requested_type, &ae_type,
		   &ae_permset, &ae_tag, &ae_id, &ae_name) == ARCHIVE_OK) {
		richace = &(richacl->a_entries[e]);

		richace->e_flags = 0;
		richace->e_mask = 0;

		switch (ae_tag) {
		case ARCHIVE_ENTRY_ACL_USER:
			ae_uid = archive_write_disk_uid(a, ae_name, ae_id);
			richace->e_id = ae_uid;
			break;
		case ARCHIVE_ENTRY_ACL_GROUP:
			ae_gid = archive_write_disk_gid(a, ae_name, ae_id);
			richace->e_id = ae_gid;
			richace->e_flags |= RICHACE_IDENTIFIER_GROUP;
			break;
		case ARCHIVE_ENTRY_ACL_USER_OBJ:
			richace->e_flags |= RICHACE_SPECIAL_WHO;
			richace->e_id = RICHACE_OWNER_SPECIAL_ID;
			break;
		case ARCHIVE_ENTRY_ACL_GROUP_OBJ:
			richace->e_flags |= RICHACE_SPECIAL_WHO;
			richace->e_id = RICHACE_GROUP_SPECIAL_ID;
			break;
		case ARCHIVE_ENTRY_ACL_EVERYONE:
			richace->e_flags |= RICHACE_SPECIAL_WHO;
			richace->e_id = RICHACE_EVERYONE_SPECIAL_ID;
			break;
		default:
			archive_set_error(a, ARCHIVE_ERRNO_MISC,
			    "Unsupported ACL tag");
			ret = ARCHIVE_FAILED;
			goto exit_free;
		}

		switch (ae_type) {
			case ARCHIVE_ENTRY_ACL_TYPE_ALLOW:
				richace->e_type =
				    RICHACE_ACCESS_ALLOWED_ACE_TYPE;
				break;
			case ARCHIVE_ENTRY_ACL_TYPE_DENY:
				richace->e_type =
				    RICHACE_ACCESS_DENIED_ACE_TYPE;
				break;
			case ARCHIVE_ENTRY_ACL_TYPE_AUDIT:
			case ARCHIVE_ENTRY_ACL_TYPE_ALARM:
				break;
		default:
			archive_set_error(a, ARCHIVE_ERRNO_MISC,
			    "Unsupported ACL entry type");
			ret = ARCHIVE_FAILED;
			goto exit_free;
		}

		for (i = 0; i < acl_nfs4_perm_map_size; ++i) {
			if (ae_permset & acl_nfs4_perm_map[i].a_perm)
				richace->e_mask |= acl_nfs4_perm_map[i].p_perm;
		}

		for (i = 0; i < acl_nfs4_flag_map_size; ++i) {
			if (ae_permset &
			    acl_nfs4_flag_map[i].a_perm)
				richace->e_flags |= acl_nfs4_flag_map[i].p_perm;
		}
	e++;
	}

	/* Fill RichACL masks */
	_richacl_mode_to_masks(richacl, mode);

	if (fd >= 0) {
		if (richacl_set_fd(fd, richacl) == 0)
			ret = ARCHIVE_OK;
		else {
			if (errno == EOPNOTSUPP) {
				/* Filesystem doesn't support ACLs */
				ret = ARCHIVE_OK;
			} else {
				archive_set_error(a, errno,
				    "Failed to set richacl on fd: %s", tname);
				ret = ARCHIVE_WARN;
			}
		}
	} else if (richacl_set_file(name, richacl) != 0) {
		if (errno == EOPNOTSUPP) {
			/* Filesystem doesn't support ACLs */
			ret = ARCHIVE_OK;
		} else {
			archive_set_error(a, errno, "Failed to set richacl: %s",
			    tname);
			ret = ARCHIVE_WARN;
		}
	}
exit_free:
	richacl_free(richacl);
	return (ret);
}
#endif /* ARCHIVE_ACL_RICHACL */

#if ARCHIVE_ACL_LIBACL
static int
set_acl(struct archive *a, int fd, const char *name,
    struct archive_acl *abstract_acl,
    int ae_requested_type, const char *tname)
{
	int		 acl_type = 0;
	int		 ae_type, ae_permset, ae_tag, ae_id;
	uid_t		 ae_uid;
	gid_t		 ae_gid;
	const char	*ae_name;
	int		 entries;
	int		 i;
	int		 ret;
	acl_t		 acl = NULL;
	acl_entry_t	 acl_entry;
	acl_permset_t	 acl_permset;

	ret = ARCHIVE_OK;
	entries = archive_acl_reset(abstract_acl, ae_requested_type);
	if (entries == 0)
		return (ARCHIVE_OK);

	switch (ae_requested_type) {
	case ARCHIVE_ENTRY_ACL_TYPE_ACCESS:
		acl_type = ACL_TYPE_ACCESS;
		break;
	case ARCHIVE_ENTRY_ACL_TYPE_DEFAULT:
		acl_type = ACL_TYPE_DEFAULT;
		break;
	default:
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

		if (acl_create_entry(&acl, &acl_entry) != 0) {
			archive_set_error(a, errno,
			    "Failed to create a new ACL entry");
			ret = ARCHIVE_FAILED;
			goto exit_free;
		}

		switch (ae_tag) {
		case ARCHIVE_ENTRY_ACL_USER:
			ae_uid = archive_write_disk_uid(a, ae_name, ae_id);
			acl_set_tag_type(acl_entry, ACL_USER);
			acl_set_qualifier(acl_entry, &ae_uid);
			break;
		case ARCHIVE_ENTRY_ACL_GROUP:
			ae_gid = archive_write_disk_gid(a, ae_name, ae_id);
			acl_set_tag_type(acl_entry, ACL_GROUP);
			acl_set_qualifier(acl_entry, &ae_gid);
			break;
		case ARCHIVE_ENTRY_ACL_USER_OBJ:
			acl_set_tag_type(acl_entry, ACL_USER_OBJ);
			break;
		case ARCHIVE_ENTRY_ACL_GROUP_OBJ:
			acl_set_tag_type(acl_entry, ACL_GROUP_OBJ);
			break;
		case ARCHIVE_ENTRY_ACL_MASK:
			acl_set_tag_type(acl_entry, ACL_MASK);
			break;
		case ARCHIVE_ENTRY_ACL_OTHER:
			acl_set_tag_type(acl_entry, ACL_OTHER);
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

		for (i = 0; i < acl_posix_perm_map_size; ++i) {
			if (ae_permset & acl_posix_perm_map[i].a_perm) {
				if (acl_add_perm(acl_permset,
				    acl_posix_perm_map[i].p_perm) != 0) {
					archive_set_error(a, errno,
					    "Failed to add ACL permission");
					ret = ARCHIVE_FAILED;
					goto exit_free;
				}
			}
		}

	}

	if (fd >= 0 && ae_requested_type == ARCHIVE_ENTRY_ACL_TYPE_ACCESS) {
		if (acl_set_fd(fd, acl) == 0)
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
	} else if (acl_set_file(name, acl_type, acl) != 0) {
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
#endif /* ARCHIVE_ACL_LIBACL */

int
archive_read_disk_entry_setup_acls(struct archive_read_disk *a,
    struct archive_entry *entry, int *fd)
{
	const char	*accpath;
	int		r;
#if ARCHIVE_ACL_LIBACL
	acl_t		acl;
#endif
#if ARCHIVE_ACL_LIBRICHACL
	struct richacl *richacl;
	mode_t		mode;
#endif

	accpath = NULL;
	r = ARCHIVE_OK;

	/* For default ACLs we need reachable accpath */
	if (*fd < 0 || S_ISDIR(archive_entry_mode(entry))) {
		accpath = archive_read_disk_entry_setup_path(a, entry, fd);
		if (accpath == NULL)
			return (ARCHIVE_WARN);
	}

	archive_entry_acl_clear(entry);

#if ARCHIVE_ACL_LIBACL
	acl = NULL;
#endif
#if ARCHIVE_ACL_LIBRICHACL
	richacl = NULL;
#endif

#if ARCHIVE_ACL_LIBRICHACL
	/* Try NFSv4 ACL first. */
	if (*fd >= 0)
		richacl = richacl_get_fd(*fd);
	else if ((!a->follow_symlinks)
	    && (archive_entry_filetype(entry) == AE_IFLNK))
		/* We can't get the ACL of a symlink, so we assume it can't
		   have one */
		richacl = NULL;
	else
		richacl = richacl_get_file(accpath);

	/* Ignore "trivial" ACLs that just mirror the file mode. */
	if (richacl != NULL) {
		mode = archive_entry_mode(entry);
		if (richacl_equiv_mode(richacl, &mode) == 0) {
			richacl_free(richacl);
			richacl = NULL;
			return (ARCHIVE_OK);
		}
	}

	if (richacl != NULL) {
		r = translate_richacl(a, entry, richacl);
		richacl_free(richacl);
		richacl = NULL;

		if (r != ARCHIVE_OK) {
			archive_set_error(&a->archive, errno,
			"Couldn't translate NFSv4 ACLs");
		}

		return (r);
	}
#endif	/* ARCHIVE_ACL_LIBRICHACL */

#if ARCHIVE_ACL_LIBACL
	/* Retrieve access ACL from file. */
	if (*fd >= 0)
		acl = acl_get_fd(*fd);
	else if ((!a->follow_symlinks)
	    && (archive_entry_filetype(entry) == AE_IFLNK))
		/* We can't get the ACL of a symlink, so we assume it can't
		   have one. */
		acl = NULL;
	else
		acl = acl_get_file(accpath, ACL_TYPE_ACCESS);

	if (acl != NULL) {
		r = translate_acl(a, entry, acl, ARCHIVE_ENTRY_ACL_TYPE_ACCESS);
		acl_free(acl);
		acl = NULL;

		if (r != ARCHIVE_OK) {
			archive_set_error(&a->archive, errno,
			    "Couldn't translate access ACLs");
			return (r);
		}
	}

	/* Only directories can have default ACLs. */
	if (S_ISDIR(archive_entry_mode(entry))) {
		acl = acl_get_file(accpath, ACL_TYPE_DEFAULT);
		if (acl != NULL) {
			r = translate_acl(a, entry, acl,
			    ARCHIVE_ENTRY_ACL_TYPE_DEFAULT);
			acl_free(acl);
			if (r != ARCHIVE_OK) {
				archive_set_error(&a->archive, errno,
				    "Couldn't translate default ACLs");
				return (r);
			}
		}
	}
#endif	/* ARCHIVE_ACL_LIBACL */
	return (r);
}

int
archive_write_disk_set_acls(struct archive *a, int fd, const char *name,
    struct archive_acl *abstract_acl, __LA_MODE_T mode)
{
	int		ret = ARCHIVE_OK;

#if !ARCHIVE_ACL_LIBRICHACL
	(void)mode;	/* UNUSED */
#endif

#if ARCHIVE_ACL_LIBRICHACL
	if ((archive_acl_types(abstract_acl)
	    & ARCHIVE_ENTRY_ACL_TYPE_NFS4) != 0) {
		ret = set_richacl(a, fd, name, abstract_acl, mode,
		    ARCHIVE_ENTRY_ACL_TYPE_NFS4, "nfs4");
	}
#if ARCHIVE_ACL_LIBACL
	else
#endif
#endif	/* ARCHIVE_ACL_LIBRICHACL */
#if ARCHIVE_ACL_LIBACL
	if ((archive_acl_types(abstract_acl)
	    & ARCHIVE_ENTRY_ACL_TYPE_POSIX1E) != 0) {
		if ((archive_acl_types(abstract_acl)
		    & ARCHIVE_ENTRY_ACL_TYPE_ACCESS) != 0) {
			ret = set_acl(a, fd, name, abstract_acl,
			    ARCHIVE_ENTRY_ACL_TYPE_ACCESS, "access");
			if (ret != ARCHIVE_OK)
				return (ret);
		}
		if ((archive_acl_types(abstract_acl)
		    & ARCHIVE_ENTRY_ACL_TYPE_DEFAULT) != 0)
			ret = set_acl(a, fd, name, abstract_acl,
			    ARCHIVE_ENTRY_ACL_TYPE_DEFAULT, "default");
	}
#endif	/* ARCHIVE_ACL_LIBACL */
	return (ret);
}
#endif /* ARCHIVE_ACL_LIBACL || ARCHIVE_ACL_LIBRICHACL */
