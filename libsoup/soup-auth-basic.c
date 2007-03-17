/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * soup-auth-basic.c: HTTP Basic Authentication
 *
 * Copyright (C) 2001-2003, Ximian, Inc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "soup-auth-basic.h"
#include "soup-headers.h"
#include "soup-message.h"
#include "soup-misc.h"
#include "soup-uri.h"

static void construct (SoupAuth *auth, const char *header);
static GSList *get_protection_space (SoupAuth *auth, const SoupUri *source_uri);
static const char *get_realm (SoupAuth *auth);
static void authenticate (SoupAuth *auth, const char *username, const char *password);
static gboolean is_authenticated (SoupAuth *auth);
static char *get_authorization (SoupAuth *auth, SoupMessage *msg);

typedef struct {
	char *realm, *token;
} SoupAuthBasicPrivate;
#define SOUP_AUTH_BASIC_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SOUP_TYPE_AUTH_BASIC, SoupAuthBasicPrivate))

G_DEFINE_TYPE (SoupAuthBasic, soup_auth_basic, SOUP_TYPE_AUTH)

static void
soup_auth_basic_init (SoupAuthBasic *basic)
{
}

static void
finalize (GObject *object)
{
	SoupAuthBasicPrivate *priv = SOUP_AUTH_BASIC_GET_PRIVATE (object);

	g_free (priv->realm);
	g_free (priv->token);

	G_OBJECT_CLASS (soup_auth_basic_parent_class)->finalize (object);
}

static void
soup_auth_basic_class_init (SoupAuthBasicClass *auth_basic_class)
{
	SoupAuthClass *auth_class = SOUP_AUTH_CLASS (auth_basic_class);
	GObjectClass *object_class = G_OBJECT_CLASS (auth_basic_class);

	g_type_class_add_private (auth_basic_class, sizeof (SoupAuthBasicPrivate));

	auth_class->scheme_name = "Basic";

	auth_class->construct = construct;
	auth_class->get_protection_space = get_protection_space;
	auth_class->get_realm = get_realm;
	auth_class->authenticate = authenticate;
	auth_class->is_authenticated = is_authenticated;
	auth_class->get_authorization = get_authorization;

	object_class->finalize = finalize;
}


static void
construct (SoupAuth *auth, const char *header)
{
	SoupAuthBasicPrivate *priv = SOUP_AUTH_BASIC_GET_PRIVATE (auth);
	GHashTable *tokens;

	header += sizeof ("Basic");

	tokens = soup_header_param_parse_list (header);
	if (!tokens)
		return;

	priv->realm = soup_header_param_copy_token (tokens, "realm");
	soup_header_param_destroy_hash (tokens);
}

static GSList *
get_protection_space (SoupAuth *auth, const SoupUri *source_uri)
{
	char *space, *p;

	space = g_strdup (source_uri->path);

	/* Strip query and filename component */
	p = strrchr (space, '/');
	if (p && p != space && p[1])
		*p = '\0';

	return g_slist_prepend (NULL, space);
}

static const char *
get_realm (SoupAuth *auth)
{
	return SOUP_AUTH_BASIC_GET_PRIVATE (auth)->realm;
}

static void
authenticate (SoupAuth *auth, const char *username, const char *password)
{
	SoupAuthBasicPrivate *priv = SOUP_AUTH_BASIC_GET_PRIVATE (auth);
	char *user_pass;
	int len;

	g_return_if_fail (username != NULL);
	g_return_if_fail (password != NULL);

	user_pass = g_strdup_printf ("%s:%s", username, password);
	len = strlen (user_pass);

	priv->token = g_base64_encode ((guchar *)user_pass, len);

	memset (user_pass, 0, len);
	g_free (user_pass);
}

static gboolean
is_authenticated (SoupAuth *auth)
{
	return SOUP_AUTH_BASIC_GET_PRIVATE (auth)->token != NULL;
}

static char *
get_authorization (SoupAuth *auth, SoupMessage *msg)
{
	SoupAuthBasicPrivate *priv = SOUP_AUTH_BASIC_GET_PRIVATE (auth);

	return g_strdup_printf ("Basic %s", priv->token);
}
