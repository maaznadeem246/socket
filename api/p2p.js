/**
 * @module P2P
 *
 * A low-level network protocol for P2P.
 *
 * @see {@link https://github.com/socketsupply/stream-relay}
 *
 */
import { createPeer } from './external/stream-relay/src/index.js'

import dgram from './dgram.js'

const Peer = createPeer(dgram)

export * from './external/stream-relay/src/index.js'
export { Peer }
