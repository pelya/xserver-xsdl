/*
   Copyright (c) 2002  XFree86 Inc
*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <assert.h>
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "swaprep.h"
#include "registry.h"
#include <X11/extensions/XResproto.h>
#include "pixmapstr.h"
#include "windowstr.h"
#include "gcstruct.h"
#include "modinit.h"
#include "protocol-versions.h"
#include "client.h"
#include "list.h"
#include "misc.h"
#include <string.h>

/** @brief Holds fragments of responses for ConstructClientIds.
 *
 *  note: there is no consideration for data alignment */
typedef struct {
    struct xorg_list l;
    int   bytes;
    /* data follows */
} FragmentList;

/** @brief Holds structure for the generated response to
           ProcXResQueryClientIds; used by ConstructClientId* -functions */
typedef struct {
    int           numIds;
    int           resultBytes;
    struct xorg_list   response;
    int           sentClientMasks[MAXCLIENTS];
} ConstructClientIdCtx;

/** @brief Allocate and add a sequence of bytes at the end of a fragment list.
           Call DestroyFragments to release the list.

    @param frags A pointer to head of an initialized linked list
    @param bytes Number of bytes to allocate
    @return Returns a pointer to the allocated non-zeroed region
            that is to be filled by the caller. On error (out of memory)
            returns NULL and makes no changes to the list.
*/
static void *
AddFragment(struct xorg_list *frags, int bytes)
{
    FragmentList *f = malloc(sizeof(FragmentList) + bytes);
    if (!f) {
        return NULL;
    } else {
        f->bytes = bytes;
        xorg_list_add(&f->l, frags->prev);
        return (char*) f + sizeof(*f);
    }
}

/** @brief Sends all fragments in the list to the client. Does not
           free anything.

    @param client The client to send the fragments to
    @param frags The head of the list of fragments
*/
static void
WriteFragmentsToClient(ClientPtr client, struct xorg_list *frags)
{
    FragmentList *it;
    xorg_list_for_each_entry(it, frags, l) {
        WriteToClient(client, it->bytes, (char*) it + sizeof(*it));
    }
}

/** @brief Frees a list of fragments. Does not free() root node.

    @param frags The head of the list of fragments
*/
static void
DestroyFragments(struct xorg_list *frags)
{
    FragmentList *it, *tmp;
    xorg_list_for_each_entry_safe(it, tmp, frags, l) {
        xorg_list_del(&it->l);
        free(it);
    }
}

/** @brief Constructs a context record for ConstructClientId* functions
           to use */
static void
InitConstructClientIdCtx(ConstructClientIdCtx *ctx)
{
    ctx->numIds = 0;
    ctx->resultBytes = 0;
    xorg_list_init(&ctx->response);
    memset(ctx->sentClientMasks, 0, sizeof(ctx->sentClientMasks));
}

/** @brief Destroys a context record, releases all memory (except the storage
           for *ctx itself) */
static void
DestroyConstructClientIdCtx(ConstructClientIdCtx *ctx)
{
    DestroyFragments(&ctx->response);
}

static int
ProcXResQueryVersion(ClientPtr client)
{
    REQUEST(xXResQueryVersionReq);
    xXResQueryVersionReply rep;

    REQUEST_SIZE_MATCH(xXResQueryVersionReq);

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.server_major = SERVER_XRES_MAJOR_VERSION;
    rep.server_minor = SERVER_XRES_MINOR_VERSION;
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.server_major);
        swaps(&rep.server_minor);
    }
    WriteToClient(client, sizeof(xXResQueryVersionReply), (char *) &rep);
    return Success;
}

static int
ProcXResQueryClients(ClientPtr client)
{
    /* REQUEST(xXResQueryClientsReq); */
    xXResQueryClientsReply rep;
    int *current_clients;
    int i, num_clients;

    REQUEST_SIZE_MATCH(xXResQueryClientsReq);

    current_clients = malloc(currentMaxClients * sizeof(int));

    num_clients = 0;
    for (i = 0; i < currentMaxClients; i++) {
        if (clients[i]) {
            current_clients[num_clients] = i;
            num_clients++;
        }
    }

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.num_clients = num_clients;
    rep.length = bytes_to_int32(rep.num_clients * sz_xXResClient);
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.num_clients);
    }
    WriteToClient(client, sizeof(xXResQueryClientsReply), (char *) &rep);

    if (num_clients) {
        xXResClient scratch;

        for (i = 0; i < num_clients; i++) {
            scratch.resource_base = clients[current_clients[i]]->clientAsMask;
            scratch.resource_mask = RESOURCE_ID_MASK;

            if (client->swapped) {
                swapl(&scratch.resource_base);
                swapl(&scratch.resource_mask);
            }
            WriteToClient(client, sz_xXResClient, (char *) &scratch);
        }
    }

    free(current_clients);

    return Success;
}

static void
ResFindAllRes(pointer value, XID id, RESTYPE type, pointer cdata)
{
    int *counts = (int *) cdata;

    counts[(type & TypeMask) - 1]++;
}

static int
ProcXResQueryClientResources(ClientPtr client)
{
    REQUEST(xXResQueryClientResourcesReq);
    xXResQueryClientResourcesReply rep;
    int i, clientID, num_types;
    int *counts;

    REQUEST_SIZE_MATCH(xXResQueryClientResourcesReq);

    clientID = CLIENT_ID(stuff->xid);

    if ((clientID >= currentMaxClients) || !clients[clientID]) {
        client->errorValue = stuff->xid;
        return BadValue;
    }

    counts = calloc(lastResourceType + 1, sizeof(int));

    FindAllClientResources(clients[clientID], ResFindAllRes, counts);

    num_types = 0;

    for (i = 0; i <= lastResourceType; i++) {
        if (counts[i])
            num_types++;
    }

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.num_types = num_types;
    rep.length = bytes_to_int32(rep.num_types * sz_xXResType);
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.num_types);
    }

    WriteToClient(client, sizeof(xXResQueryClientResourcesReply),
                  (char *) &rep);

    if (num_types) {
        xXResType scratch;
        const char *name;

        for (i = 0; i < lastResourceType; i++) {
            if (!counts[i])
                continue;

            name = LookupResourceName(i + 1);
            if (strcmp(name, XREGISTRY_UNKNOWN))
                scratch.resource_type = MakeAtom(name, strlen(name), TRUE);
            else {
                char buf[40];

                snprintf(buf, sizeof(buf), "Unregistered resource %i", i + 1);
                scratch.resource_type = MakeAtom(buf, strlen(buf), TRUE);
            }

            scratch.count = counts[i];

            if (client->swapped) {
                swapl(&scratch.resource_type);
                swapl(&scratch.count);
            }
            WriteToClient(client, sz_xXResType, (char *) &scratch);
        }
    }

    free(counts);

    return Success;
}

static unsigned long
ResGetApproxPixmapBytes(PixmapPtr pix)
{
    unsigned long nPixels;
    int bytesPerPixel;

    bytesPerPixel = pix->drawable.bitsPerPixel >> 3;
    nPixels = pix->drawable.width * pix->drawable.height;

    /* Divide by refcnt as pixmap could be shared between clients,  
     * so total pixmap mem is shared between these. 
     */
    return (nPixels * bytesPerPixel) / pix->refcnt;
}

static void
ResFindPixmaps(pointer value, XID id, pointer cdata)
{
    unsigned long *bytes = (unsigned long *) cdata;
    PixmapPtr pix = (PixmapPtr) value;

    *bytes += ResGetApproxPixmapBytes(pix);
}

static void
ResFindWindowPixmaps(pointer value, XID id, pointer cdata)
{
    unsigned long *bytes = (unsigned long *) cdata;
    WindowPtr pWin = (WindowPtr) value;

    if (pWin->backgroundState == BackgroundPixmap)
        *bytes += ResGetApproxPixmapBytes(pWin->background.pixmap);

    if (pWin->border.pixmap != NULL && !pWin->borderIsPixel)
        *bytes += ResGetApproxPixmapBytes(pWin->border.pixmap);
}

static void
ResFindGCPixmaps(pointer value, XID id, pointer cdata)
{
    unsigned long *bytes = (unsigned long *) cdata;
    GCPtr pGC = (GCPtr) value;

    if (pGC->stipple != NULL)
        *bytes += ResGetApproxPixmapBytes(pGC->stipple);

    if (pGC->tile.pixmap != NULL && !pGC->tileIsPixel)
        *bytes += ResGetApproxPixmapBytes(pGC->tile.pixmap);
}

static int
ProcXResQueryClientPixmapBytes(ClientPtr client)
{
    REQUEST(xXResQueryClientPixmapBytesReq);
    xXResQueryClientPixmapBytesReply rep;
    int clientID;
    unsigned long bytes;

    REQUEST_SIZE_MATCH(xXResQueryClientPixmapBytesReq);

    clientID = CLIENT_ID(stuff->xid);

    if ((clientID >= currentMaxClients) || !clients[clientID]) {
        client->errorValue = stuff->xid;
        return BadValue;
    }

    bytes = 0;

    FindClientResourcesByType(clients[clientID], RT_PIXMAP, ResFindPixmaps,
                              (pointer) (&bytes));

    /* 
     * Make sure win background pixmaps also held to account. 
     */
    FindClientResourcesByType(clients[clientID], RT_WINDOW,
                              ResFindWindowPixmaps, (pointer) (&bytes));

    /* 
     * GC Tile & Stipple pixmaps too.
     */
    FindClientResourcesByType(clients[clientID], RT_GC,
                              ResFindGCPixmaps, (pointer) (&bytes));

#ifdef COMPOSITE
    /* FIXME: include composite pixmaps too */
#endif

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;
    rep.bytes = bytes;
#ifdef _XSERVER64
    rep.bytes_overflow = bytes >> 32;
#else
    rep.bytes_overflow = 0;
#endif
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.bytes);
        swapl(&rep.bytes_overflow);
    }
    WriteToClient(client, sizeof(xXResQueryClientPixmapBytesReply),
                  (char *) &rep);

    return Success;
}

/** @brief Finds out if a client's information need to be put into the
    response; marks client having been handled, if that is the case.

    @param client   The client to send information about
    @param mask     The request mask (0 to send everything, otherwise a
                    bitmask of X_XRes*Mask)
    @param ctx      The context record that tells which clients and id types
                    have been already handled
    @param sendMask Which id type are we now considering. One of X_XRes*Mask.

    @return Returns TRUE if the client information needs to be on the
            response, otherwise FALSE.
*/
static Bool
WillConstructMask(ClientPtr client, CARD32 mask,
                  ConstructClientIdCtx *ctx, int sendMask)
{
    if ((!mask || (mask & sendMask))
        && !(ctx->sentClientMasks[client->index] & sendMask)) {
        ctx->sentClientMasks[client->index] |= sendMask;
        return TRUE;
    } else {
        return FALSE;
    }
}

/** @brief Constructs a response about a single client, based on a certain
           client id spec

    @param sendClient Which client wishes to receive this answer. Used for
                      byte endianess.
    @param client     Which client are we considering.
    @param mask       The client id spec mask indicating which information
                      we want about this client.
    @param ctx        The context record containing the constructed response
                      and information on which clients and masks have been
                      already handled.

    @return Return TRUE if everything went OK, otherwise FALSE which indicates
            a memory allocation problem.
*/
static Bool
ConstructClientIdValue(ClientPtr sendClient, ClientPtr client, CARD32 mask,
                       ConstructClientIdCtx *ctx)
{
    xXResClientIdValue rep;

    rep.spec.client = client->clientAsMask;
    if (client->swapped) {
        swapl (&rep.spec.client);
    }

    if (WillConstructMask(client, mask, ctx, X_XResClientXIDMask)) {
        void *ptr = AddFragment(&ctx->response, sizeof(rep));
        if (!ptr) {
            return FALSE;
        }

        rep.spec.mask = X_XResClientXIDMask;
        rep.length = 0;
        if (sendClient->swapped) {
            swapl (&rep.spec.mask);
            /* swapl (&rep.length, n); - not required for rep.length = 0 */
        }

        memcpy(ptr, &rep, sizeof(rep));

        ctx->resultBytes += sizeof(rep);
        ++ctx->numIds;
    }
    if (WillConstructMask(client, mask, ctx, X_XResLocalClientPIDMask)) {
        pid_t pid = GetClientPid(client);

        if (pid != -1) {
            void *ptr = AddFragment(&ctx->response,
                                    sizeof(rep) + sizeof(CARD32));
            CARD32 *value = (void*) ((char*) ptr + sizeof(rep));

            if (!ptr) {
                return FALSE;
            }

            rep.spec.mask = X_XResLocalClientPIDMask;
            rep.length = 4;

            if (sendClient->swapped) {
                swapl (&rep.spec.mask);
                swapl (&rep.length);
            }

            if (sendClient->swapped) {
                swapl (value);
            }
            memcpy(ptr, &rep, sizeof(rep));
            *value = pid;

            ctx->resultBytes += sizeof(rep) + sizeof(CARD32);
            ++ctx->numIds;
        }
    }

    /* memory allocation errors earlier may return with FALSE */
    return TRUE;
}

/** @brief Constructs a response about all clients, based on a client id specs

    @param client   Which client which we are constructing the response for.
    @param numSpecs Number of client id specs in specs
    @param specs    Client id specs

    @return Return Success if everything went OK, otherwise a Bad* (currently
            BadAlloc or BadValue)
*/
static int
ConstructClientIds(ClientPtr client,
                   int numSpecs, xXResClientIdSpec* specs,
                   ConstructClientIdCtx *ctx)
{
    int specIdx;

    for (specIdx = 0; specIdx < numSpecs; ++specIdx) {
        if (specs[specIdx].client == 0) {
            int c;
            for (c = 0; c < currentMaxClients; ++c) {
                if (clients[c]) {
                    if (!ConstructClientIdValue(client, clients[c],
                                                specs[specIdx].mask, ctx)) {
                        return BadAlloc;
                    }
                }
            }
        } else {
            int clientID = CLIENT_ID(specs[specIdx].client);

            if ((clientID < currentMaxClients) && clients[clientID]) {
                if (!ConstructClientIdValue(client, clients[clientID],
                                            specs[specIdx].mask, ctx)) {
                    return BadAlloc;
                }
            }
        }
    }

    /* memory allocation errors earlier may return with BadAlloc */
    return Success;
}

/** @brief Response to XResQueryClientIds request introduced in XResProto v1.2

    @param client Which client which we are constructing the response for.

    @return Returns the value returned from ConstructClientIds with the same
            semantics
*/
static int
ProcXResQueryClientIds (ClientPtr client)
{
    REQUEST(xXResQueryClientIdsReq);

    xXResQueryClientIdsReply  rep;
    xXResClientIdSpec        *specs = (void*) ((char*) stuff + sizeof(*stuff));
    int                       rc;
    ConstructClientIdCtx      ctx;

    InitConstructClientIdCtx(&ctx);

    REQUEST_AT_LEAST_SIZE(xXResQueryClientIdsReq);
    REQUEST_FIXED_SIZE(xXResQueryClientIdsReq,
                       stuff->numSpecs * sizeof(specs[0]));

    rc = ConstructClientIds(client, stuff->numSpecs, specs, &ctx);

    if (rc == Success) {
        rep.type = X_Reply;
        rep.sequenceNumber = client->sequence;

        assert((ctx.resultBytes & 3) == 0);
        rep.length = bytes_to_int32(ctx.resultBytes);
        rep.numIds = ctx.numIds;

        if (client->swapped) {
            swaps (&rep.sequenceNumber);
            swapl (&rep.length);
            swapl (&rep.numIds);
        }

        WriteToClient(client,sizeof(rep),(char*)&rep);
        WriteFragmentsToClient(client, &ctx.response);
    }

    DestroyConstructClientIdCtx(&ctx);

    return rc;
}

static int
ProcResDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_XResQueryVersion:
        return ProcXResQueryVersion(client);
    case X_XResQueryClients:
        return ProcXResQueryClients(client);
    case X_XResQueryClientResources:
        return ProcXResQueryClientResources(client);
    case X_XResQueryClientPixmapBytes:
        return ProcXResQueryClientPixmapBytes(client);
    case X_XResQueryClientIds:
        return ProcXResQueryClientIds(client);
    case X_XResQueryResourceBytes:
        /* not implemented yet */
        return BadRequest;
    default: break;
    }

    return BadRequest;
}

static int
SProcXResQueryVersion(ClientPtr client)
{
    REQUEST(xXResQueryVersionReq);
    REQUEST_SIZE_MATCH(xXResQueryVersionReq);
    return ProcXResQueryVersion(client);
}

static int
SProcXResQueryClientResources(ClientPtr client)
{
    REQUEST(xXResQueryClientResourcesReq);
    REQUEST_SIZE_MATCH(xXResQueryClientResourcesReq);
    swapl(&stuff->xid);
    return ProcXResQueryClientResources(client);
}

static int
SProcXResQueryClientPixmapBytes(ClientPtr client)
{
    REQUEST(xXResQueryClientPixmapBytesReq);
    REQUEST_SIZE_MATCH(xXResQueryClientPixmapBytesReq);
    swapl(&stuff->xid);
    return ProcXResQueryClientPixmapBytes(client);
}

static int
SProcXResQueryClientIds (ClientPtr client)
{
    REQUEST(xXResQueryClientIdsReq);

    REQUEST_AT_LEAST_SIZE (xXResQueryClientIdsReq);
    swapl(&stuff->numSpecs);
    return ProcXResQueryClientIds(client);
}

static int
SProcResDispatch (ClientPtr client)
{
    REQUEST(xReq);
    swaps(&stuff->length);

    switch (stuff->data) {
    case X_XResQueryVersion:
        return SProcXResQueryVersion(client);
    case X_XResQueryClients:   /* nothing to swap */
        return ProcXResQueryClients(client);
    case X_XResQueryClientResources:
        return SProcXResQueryClientResources(client);
    case X_XResQueryClientPixmapBytes:
        return SProcXResQueryClientPixmapBytes(client);
    case X_XResQueryClientIds:
        return SProcXResQueryClientIds(client);
    case X_XResQueryResourceBytes:
        /* not implemented yet */
        return BadRequest;
    default: break;
    }

    return BadRequest;
}

void
ResExtensionInit(INITARGS)
{
    (void) AddExtension(XRES_NAME, 0, 0,
                        ProcResDispatch, SProcResDispatch,
                        NULL, StandardMinorOpcode);
}
