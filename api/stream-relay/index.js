/**
 * @module Peer
 * @status Experimental
 *
 *Provides Network Peer

 *> Note: The code in the module may change a lot in the next few weeks but the
 *API will continue to be backward compatible thoughout all future releases.
 *
 */

import { isBufferLike } from '../util.js'
import { Buffer } from '../buffer.js'
import { sodium } from '../crypto.js'
import { Cache } from './cache.js'
import { Encryption } from './encryption.js'
import { PTP } from './scheme-ptp.js'

import {
  Packet,
  PacketPing,
  PacketPong,
  PacketIntro,
  PacketAsk,
  PacketAnswer,
  PacketPublish,
  PacketJoin,
  PacketQuery,
  addHops,
  sha256
} from './packets.js'

export { sha256, Cache, Encryption }
export const PING_RETRY = 500
export const DEFAULT_PORT = 9777
export const DEFAULT_TEST_PORT = 9778
export const KEEP_ALIVE = 29_000

export const portGenerator = (localPort, testPort) => {
  const portCache = { [localPort]: true, [testPort]: true }
  const next = () => ~~(Math.random() * 0xffff) || 1

  return (ports = portCache, p) => {
    do ports[p = next()] = true; while (!ports[p] && p)
    return p
  }
}

export class RemotePeer {
  peerId = null
  address = null
  port = 0

  natType = null
  clusterId = null
  pingId = null
  distance = 0
  geospacial = null
  publicKey = null
  privateKey = null
  lastUpdate = 0
  lastRequest = 0

  constructor (o) {
    if (!o.port) throw new Error('expected .port')
    if (!o.address) throw new Error('expected .address')
    if (!o.peerId) throw new Error('expected .peerId')

    if (!o.clusterId) this.natType = 'static'
    if (o.publicKey) this.publicKey = o.publicKey
    if (o.privateKey) this.privateKey = o.privateKey

    Object.assign(this, o)
  }
}

export const createPeer = udp => {
  class Peer {
    port = null
    address = null
    natType = null
    clusterId = null
    reflectionId = null
    peerId = ''
    ctime = Date.now()
    lastUpdate = 0
    closing = false
    listening = false
    unpublished = {}
    cache = null
    queries = {}
    limits = {}
    clock = 0
    maxHops = 16
    bdpCache = []

    peers = JSON.parse(/* snapshot_start=1681336577824, filter=easy,static */`[
      {"address":"44.213.42.133","port":10885,"peerId":"4825fe0475c44bc0222e76c5fa7cf4759cd5ef8c66258c039653f06d329a9af5"},
      {"address":"107.20.123.15","port":31503,"peerId":"2de8ac51f820a5b9dc8a3d2c0f27ccc6e12a418c9674272a10daaa609eab0b41"},
      {"address":"54.227.171.107","port":43883,"peerId":"7aa3d21ceb527533489af3888ea6d73d26771f30419578e85fba197b15b3d18d"},
      {"address":"54.157.134.116","port":34420,"peerId":"1d2315f6f16e5f560b75fbfaf274cad28c12eb54bb921f32cf93087d926f05a9"},
      {"address":"184.169.205.9","port":52489,"peerId":"db00d46e23d99befe42beb32da65ac3343a1579da32c3f6f89f707d5f71bb052"},
      {"address":"35.158.123.13","port":31501,"peerId":"4ba1d23266a2d2833a3275c1d6e6f7ce4b8657e2f1b8be11f6caf53d0955db88"},
      {"address":"35.160.18.99","port":11205,"peerId":"2bad1ff23dd5e0e32d3d85e1d9bb6324fef54fec0224c4f132066ad3d0096a35"},
      {"address":"3.77.22.13","port":40501,"peerId":"1998b9e5e0b05ae7865545499da298d2064afe07c36187fd4047c603a4ac2d7"},
      {"address":"3.68.89.3","port":22787,"peerId":"448b083bd8a495ce684d5837359ce69d0ff8a5a844efe18583ab000c99d3a0ff"},
      {"address":"3.76.100.161","port":25761,"peerId":"07bffa90d89bf74e06ff7f83938b90acb1a1c5ce718d1f07854c48c6c12cee49"},
      {"address":"3.70.241.230","port":61926,"peerId":"1d7ee8d965794ee286ac425d060bab27698a1de92986dc6f4028300895c6aa5c"},
      {"address":"3.70.160.181","port":41141,"peerId":"707c07171ac9371b2f1de23e78dad15d29b56d47abed5e5a187944ed55fc8483"},
      {"address":"3.122.250.236","port":64236,"peerId":"a830615090d5cdc3698559764e853965a0d27baad0e3757568e6c7362bc6a12a"},
      {"address":"18.130.98.23","port":25111,"peerId":"ba483c1477ab7a99de2d9b60358d9641ff6a6dc6ef4e3d3e1fc069b19ac89da4"},
      {"address":"18.129.76.26","port":9210,"peerId":"099b00e0e516b2f1599459b2429d0dbb3a08eeadce00e68bbc4a7e3f2171bb65"},
      {"address":"13.230.254.25","port":65049,"peerId":"592e9fd43abcf854fa5cc9a203bebd15afb2e900308044b77599cd8f46f2532d"},
      {"address":"18.138.72.20","port":2909,"peerId":"199b00e0e516b2f1599459b2429d0dbb3a08eeadce00e68bbc4a7e3f2171bb65"},
      {"address":"18.229.140.216","port":36056,"peerId":"73d726c04c05fb3a8a5382e7a4d7af41b4e1661aadf9020545f23781fefe3527"}
    ]`/* snapshot_end=1681336679337 */).map(o => new RemotePeer(o))

    static async create (...args) {
      await sodium.ready
      return new this(...args)
    }

    constructor (persistedState) {
      if (persistedState && !persistedState.config) {
        persistedState = { config: persistedState }
      }

      const { config = {} } = persistedState

      this.encryption = new Encryption(config.keys)

      if (!config.peerId) config.peerId = this.encryption.publicKey
      if (config.scheme === 'PTP') PTP.init(this)

      this.config = {
        port: DEFAULT_PORT,
        keepalive: KEEP_ALIVE,
        testPort: DEFAULT_TEST_PORT,
        lastGroupBroadcast: -1,
        ...config
      }

      let cacheData

      if (persistedState?.data?.length > 0) {
        cacheData = new Map(persistedState.data)
      }

      this.cache = new Cache(cacheData, config.siblingResolver, this.reject)

      this.unpublished = persistedState?.unpublished || {}
      this.onError = (err) => { console.error(err) }

      Object.assign(this, config)
      this.introducer = !!config.introducer
      this.natType = config.introducer ? 'static' : null
      this.port = null
      this.address = config.address || null

      if (config.introducer) this.port = config.port || DEFAULT_PORT

      this.socket = udp.createSocket('udp4', this)
      this.testSocket = udp.createSocket('udp4', this)
    }

    async init () {
      return new Promise(resolve => {
        this.socket.on('listening', () => {
          this.listening = true
          if (this.onListening) this.onListening()
          this.requestReflection()
          resolve()
        })

        this.socket.on('message', (...args) => this.onMessage(...args))
        this.socket.on('error', (...args) => this.onError(...args))
        this.testSocket.on('message', (...args) => this.onMessage(...args))
        this.testSocket.on('error', (...args) => this.onError(...args))

        this.socket.setMaxListeners(2048)
        this.testSocket.setMaxListeners(2048)

        this.socket.bind(this.config.port)
        this.testSocket.bind(this.config.testPort)

        globalThis.window?.addEventListener('online', async () => {
          await this.join()

          setTimeout(async () => {
            for (const [packetId, clusterId] of Object.entries(this.unpublished)) {
              const packet = await this.cache.get(packetId)
              if (packet) this.mcast(packet, packetId, clusterId)
            }
          }, 1024)
        })

        const keepaliveHandler = async ts => {
          if (this.closing) return false // cancel timer

          const offline = globalThis.navigator && !globalThis.navigator.onLine
          if (!offline && !this.config.introducer) this.requestReflection()

          if (this.onInterval) this.onInterval()

          for (const [k, packet] of [...this.cache.data]) {
            const TTL = Packet.ttl

            if (packet.timestamp + TTL < ts) {
              await this.mcast(packet, packet.packetId, packet.clusterId, true)
              this.cache.delete(k)
              if (this.onDelete) this.onDelete(packet)
            }
          }

          for (const k of Object.keys(this.queries)) {
            if (--this.queries[k] === 0) delete this.queries[k]
          }

          // debug('PING HART', this.peerId, this.peers)
          for (const [i, peer] of Object.entries(this.peers)) {
            if (peer.clusterId) {
              const then = Date.now() - peer.lastUpdate

              if (then >= this.config.keepalive) {
                this.peers.splice(i, 1)
                continue
              }
            }

            if (this.limits[peer.address]) {
              this.limits[peer.address] = this.limits[peer.address] / 1.05
            }

            this.ping(peer, {
              peerId: this.peerId,
              natType: this.natType,
              cacheSize: this.cache.size,
              clusterId: this.clusterId,
              isHeartbeat: true
            })
          }
        }

        this.timer(0, this.config.keepalive, keepaliveHandler)
      })
    }

    async send (data, ...args) {
      try {
        await this.socket.send(data, ...args)
        return true
      } catch (err) {
        this.onError(new Error('ESEND'), ...args)
        return false
      }
    }

    getState () {
      return {
        config: this.config,
        data: [...this.cache.data],
        unpublished: this.unpublished
      }
    }

    getPeers (packet, peers, n = 3) {
      let list = peers

      // if our peer is behind a hard NAT, filter out peers with the same NATs
      if (this.natType === 'hard') {
        list = list.filter(p => p.natType !== 'hard' || !p.natType)
      }

      const candidates = list
        .filter(p => p.natType !== null) // some may not be known yet
        .sort((a, b) => a.distance - b.distance)
        .slice(0, 16)
        .sort(() => Math.random() - 0.5)
        .slice(0, n)

      const members = peers.filter(p => {
        return p.clusterId === this.clusterId && !candidates.find(p => p.peerId)
      })

      if (members.length) {
        if (candidates.length === n) {
          candidates[0] = members[0]
        } else {
          candidates.push(members[0])
        }
      }

      return candidates
    }

    // Packets MAY be addressed directly to a peer using IP address and port,
    // but should never assume that any particular peer will be available.
    // Instead, packets should be multicast (not broadcast) to the network.
    async mcast (packet, packetId, clusterId, isTaxed, ignorelist = []) {
      let list = this.peers

      if (Array.isArray(packet.message?.history)) {
        ignorelist = [...ignorelist, packet.message.history]
      }

      if (ignorelist.length) {
        list = list.filter(p => !ignorelist.find(peer => {
          return p.address === peer.address && p.port === peer.port
        }))
      }

      const peers = this.getPeers(packet, list)
      const data = await Packet.encode(packet)

      for (const peer of peers) {
        this.timer(10, 10, async () => {
          if (await this.send(isTaxed ? addHops(data) : data, peer.port, peer.address)) {
            delete this.unpublished[packetId]
          }
        })
      }
    }

    timer (delay, repeat, fn) {
      if (this.closing) return false

      let tid = null
      const interval = () => {
        if (fn(Date.now()) === false) clearInterval(tid)
      }

      if (!delay && fn(Date.now()) !== false && repeat) {
        tid = setInterval(interval, repeat)
        return
      }

      setTimeout(() => {
        if (fn(Date.now()) === false) return
        if (!repeat) return
        tid = setInterval(interval, repeat)
      }, delay)
    }

    requestReflection (reflectionId) {
      // this.reflectionId will have to get unassigned at some point to allow this to try again
      if (this.closing || this.config.introducer) return

      // get two random easy peers from the known peers and send pings, we can
      // learn this peer's nat type if we have at least two Pongs from peers
      // that are outside of this peer's NAT.
      const peers = this.peers.filter(p => p.natType !== 'static' || p.natType !== 'easy')

      const [peer1, peer2] = peers.sort(() => Math.random() - 0.5)

      const peerId = this.peerId
      const pingId = Math.random().toString(16).slice(2)
      const testPort = this.config.testPort

      // debug(`  requestReflection peerId=${peerId}, pingId=${pingId}`)
      this.ping(peer1, { peerId, pingId, isReflection: true })
      this.ping(peer2, { peerId, pingId, isReflection: true, testPort })
    }

    async ping (peer, props = {}) {
      if (!peer) return

      const packet = new PacketPing({
        clusterId: this.clusterId,
        message: { peerId: peer.peerId, ...props }
      })

      const data = await Packet.encode(packet)

      const retry = async () => {
        if (this.closing) return false

        const p = this.peers.find(p => p.peerId === peer.peerId)
        if (p && p.pingId === packet.message.pingId) return false
        await this.send(data, peer.port, peer.address)
        if (p) p.lastRequest = Date.now()
      }

      await retry()

      this.timer(PING_RETRY, 0, retry)
      this.timer(PING_RETRY * 8, 0, retry)
      this.timer(PING_RETRY * 16, 0, retry)

      return packet
    }

    setPeer (packet, port, address) {
      if (this.address === address && this.port === port) return this
      const { peerId, natType } = packet.message

      const existingPeer = this.peers.find(p => p.peerId === peerId)

      const newPeer = {
        peerId,
        port,
        address,
        clusterId: packet.message.clusterId,
        lastUpdate: Date.now(),
        distance: 0
      }

      if (natType) newPeer.natType = natType
      newPeer.clusterId = packet.clusterId

      if (!existingPeer) {
        try {
          const peer = new RemotePeer(newPeer)

          if (this.peers.length > 4096) {
            const oldest = [...this.peers].sort((a, b) => {
              return a.lastUpdate - b.lastUpdate
            })
            const index = this.peers.find(p => p.peerId === oldest.peerId)
            this.peers.splice(index, 1)
          }

          this.peers.push(peer)
          return peer
        } catch (err) {
          return null
        }
      }

      newPeer.distance = existingPeer.lastUpdate - existingPeer.lastRequest
      Object.assign(existingPeer, newPeer)
      return existingPeer
    }

    //
    // users call this at least once, when their app starts. It will mcast
    // this peer, their latest clusters and starts querying toe network for peers.
    //
    async join () {
      if (!this.port) return

      const packet = new PacketJoin({
        clusterId: this.clusterId,
        message: {
          peerId: this.peerId,
          natType: this.natType,
          geospatial: null, // hold
          address: this.address,
          port: this.port
        }
      })

      const data = await Packet.encode(packet)
      const sends = []

      for (const peer of this.getPeers(packet, this.peers)) {
        sends.push(this.send(data, peer.port, peer.address))
      }

      await Promise.all(sends)
    }

    async publish (args) {
      let { peerId, clusterId, message, to, packet, nextId, meta = {} } = args

      if (!to) throw new Error('.to field required to publish')
      to = Buffer.from(to).toString('base64')

      if (message && typeof message === 'object' && !isBufferLike(message)) {
        try {
          message = Buffer.from(JSON.stringify(message))
        } catch {}
      }

      if (this.prepareMessage) message = this.prepareMessage(message, peerId)
      else if (this.encryption.has(to)) message = this.encryption.seal(message, to)

      let messages = [message]
      const len = message?.byteLength ?? message?.length ?? 0

      let clock = packet ? packet.clock : 0

      const siblings = [...this.cache.data.values()].filter(p => {
        return p.previousId === packet?.packetId
      })

      if (siblings.length) {
        // if there are siblings of the previous packet
        // pick the highest clock value, the parent packet or the sibling
        const sort = (a, b) => a.clock - b.clock
        const sib = siblings.sort(sort).reverse()[0]
        clock = Math.max(clock, sib.clock) + 1
      }

      if (len > 1024) { // Split packets that have messages bigger than Packet.maxLength
        messages = [{
          meta,
          peerId: this.peerId,
          size: message.length,
          indexes: Math.ceil(message.length / 1024)
        }]
        let pos = 0
        while (pos < message.length) messages.push(message.slice(pos, pos += 1024))
      }

      // turn each message into an actual packet
      const packets = messages.map(message => new PacketPublish({
        to,
        clusterId,
        clock: ++clock,
        message
      }))

      if (packet) packets[0].previousId = packet.packetId
      if (nextId) packets[packets.length - 1].nextId = nextId

      // set the .packetId (any maybe the .previousId and .nextId)
      for (let i = 0; i < packets.length; i++) {
        if (packets.length > 1) packets[i].index = i

        if (i === 0) {
          packets[0].packetId = await sha256(packets[0].message)
        } else {
          // all fragments will have the same previous packetId
          // the index is used to stitch them back together in order.
          packets[i].previousId = packets[0].packetId
        }

        if (packets[i + 1]) {
          packets[i + 1].packetId = await sha256(
            await sha256(packets[i].packetId) +
            await sha256(packets[i + 1].message)
          )
          packets[i].nextId = packets[i + 1].packetId
        }
      }

      for (const packet of packets) {
        const inserted = await this.cache.insert(packet.packetId, packet)

        if (inserted && this.onPacket) {
          this.onPacket(packet, this.port, this.address)
        }

        this.unpublished[packet.packetId] = clusterId
        if (globalThis.navigator && !globalThis.navigator.onLine) continue

        await this.mcast(packet, packet.packetId, clusterId)
      }

      return packets
    }

    async negotiateCache (peer, packet, port, address) {
      const packets = [...this.cache.data.values()].sort((a, b) => a.hops - b.hops)

      // the peer has no data, don't ask them anything, tell them everything
      if (packet.message?.cacheSize === 0) {
        return Promise.all(packets.map(async (p, i) => {
          return this.send(await Packet.encode(p), port, address)
        }))
      }

      // tell the other peer about what we know we need for sure.

      const toSend = [] // don't send tails if they are also heads

      // a tail is a packet with a .previousId that we don't have, the purpose
      // of sending it is to request packets that came before these.
      this.cache.tails((p, i) => {
        toSend.push(new PacketAsk({
          packetId: p.previousId,
          clusterId: p.clusterId,
          message: { tail: true, peerId: this.peerId }
        }))
      })

      // a head is the last packet we know about in the chain, the purpose
      // of sending it is to ask for packets that might come after these.
      this.cache.heads((p, i) => {
        if (toSend.includes(p.packetId)) return
        toSend.push(new PacketAsk({
          packetId: p.packetId,
          clusterId: p.clusterId,
          message: { peerId: this.peerId }
        }))
      })

      let count = 0
      for (const packet of packets) {
        if (count >= 3) break

        if (!toSend.find(p => p.packetId === packet.packetId)) {
          count++
          toSend.push(packet)
        }
      }

      const sends = toSend.map(packet => Packet.encode(packet).then(data => {
        return this.send(data, port, address)
      }))

      return Promise.all(sends)
    }

    async onAskPacket (packet, port, address) {
      const reply = async p => {
        p = { ...p, type: PacketAnswer.type }
        this.send(await Packet.encode(p), port, address)
      }

      if (packet.message.tail) { // move backward from the specified packetId
        const p = await this.cache.get(packet.packetId)
        if (p) reply(p)
      } else { // move forward from the specified packetId
        this.cache.each(reply, { packetId: packet.message.packetId })
      }
    }

    async onAnswerPacket (packet, port, address) {
      packet = { ...packet, type: PacketPublish.type }

      if (await this.cache.insert(packet.packetId, packet)) {
        if (this.onPub) this.onPub(packet, port, address)

        // we may want to continue this conversation if the peer responded
        // successfully with an answer and we are still missing data.
        const isMissingTail = packet.previousId && !await this.cache.get(packet.previousId)
        const isMissingHead = packet.previousId && !await this.cache.get(packet.nextId)

        if (isMissingTail) {
          await this.send(await Packet.encode(new PacketAsk({
            clusterId: packet.clusterId,
            packetId: packet.previousId,
            message: { tail: true }
          })), port, address)
        }

        if (isMissingHead) {
          await this.send(await Packet.encode(new PacketAsk({
            clusterId: packet.clusterId,
            packetId: packet.nextId,
            message: {}
          })), port, address)
        }
      }
    }

    async onQueryPacket (packet, port, address) {
      if (this.queries[packet.packetId]) return
      this.queries[packet.packetId] = 32 // TTL of 32x30s

      const { packetId, clusterId } = packet

      if (this.onQuery && !await this.onQuery(packet, port, address)) {
        if (!Array.isArray(packet.message.history)) {
          packet.message.history = []
        }

        packet.message.history.push({ address: this.address, port: this.port })
        if (packet.message.history.length >= 16) packet.message.history.shift()

        // await for async call-stack
        return await this.mcast(packet, packetId, clusterId, true)
      }
    }

    close () {
      this.closing = true
    }

    //
    // Events
    //
    onConnection (peer, packet, port, address) {
      if (this.closing || this.config.introducer) return
      this.negotiateCache(peer, packet, port, address)
      if (this.onConnect) this.onConnect(peer, packet, port, address)
    }

    async onPing (packet, port, address) {
      this.lastUpdate = Date.now()
      const { pingId, isReflection, isConnection, clusterId, isHeartbeat, testPort } = packet.message
      const peer = this.setPeer(packet, port, address)

      if (isHeartbeat) {
        if (clusterId) await this.negotiateCache(peer, packet, port, address)
        return
      }

      const message = {
        peerId: this.peerId,
        cacheSize: this.cache.size,
        port,
        address
      }

      if (pingId) message.pingId = pingId

      if (isReflection) {
        message.isReflection = true
        message.port = port
        message.address = address
      } else {
        message.natType = this.natType
      }

      if (isConnection && peer) {
        message.isConnection = true
        message.port = this.port
        message.address = this.address
        this.onConnection(peer, packet, port, address)
      }

      const packetPong = new PacketPong({ clusterId: this.clusterId, message })
      const buf = await Packet.encode(packetPong)

      await this.send(buf, port, address)

      if (testPort) { // also send to the test port
        message.testPort = testPort
        const packetPong = new PacketPong({ clusterId: this.clusterId, message })
        const buf = await Packet.encode(packetPong)
        await this.send(buf, testPort, address)
      }
    }

    async onPong (packet, port, address) {
      this.lastUpdate = Date.now()
      const { testPort, pingId, isReflection } = packet.message
      const oldType = this.natType

      // debug('<- PONG', this.address, this.port, this.peerId, '<>', packet.message.peerId, address, port)

      if (packet.message.isConnection) {
        const peer = this.setPeer(packet, packet.message.port, packet.message.address)
        if (!peer) return
        peer.lastUpdate = Date.now()
        if (pingId) peer.pingId = pingId

        this.onConnection(peer, packet, port, address)
        return
      }

      if (isReflection && this.natType !== 'static') {
        // this isn't a connection, it's just verifying that this is or is not a static peer
        if (packet.message.address === this.address && testPort === this.config.testPort) {
          this.reflectionId = null
          this.natType = 'static'
          this.address = packet.message.address
          this.port = packet.message.port
          // debug(`  response ${this.peerId} nat=${this.natType}`)
          if (this.onNat && oldType !== this.natType) this.onNat(this.natType)
          this.join()
          return
        }

        if (!testPort && pingId && this.reflectionId === null) {
          this.reflectionId = pingId
        } else if (!testPort && pingId && pingId === this.reflectionId) {
          if (packet.message.address === this.address && packet.message.port === this.port) {
            this.natType = 'easy'
          } else if (packet.message.address === this.address && packet.message.port !== this.port) {
            this.natType = 'hard'
          }

          this.reflectionId = null

          this.ping(this, {
            peerId: this.peerId,
            natType: this.natType,
            cacheSize: this.cache.size,
            isHeartbeat: true
          })

          if (oldType !== this.natType) {
            // debug(`  response ${this.peerId} nat=${this.natType}`)
            if (this.onNat) this.onNat(this.natType)
            this.join() // this peer can let the user know we now know our nat type
          }
        }

        this.address = packet.message.address
        this.port = packet.message.port
      } else if (!this.peers.find(p => p.address === address && p.port === port)) {
        const peer = this.setPeer(packet, port, address)
        if (peer) this.onConnection(peer, packet, port, address)
      } else {
        this.setPeer(packet, port, address)
      }
    }

    //
    // The caller is offering us a peer to connect with
    //
    async onIntro (packet, port, address) {
      const peer = this.setPeer(packet, packet.message.port, packet.message.address)
      if (!peer || peer.connecting) return

      this.negotiateCache(peer, packet, port, address)

      const remoteHard = packet.message.natType === 'hard'
      const localHard = this.natType === 'hard'
      const localEasy = ['easy', 'static'].includes(this.natType)
      const remoteEasy = ['easy', 'static'].includes(packet.message.natType)

      // debug(`intro arriving at ${this.peerId} (${this.address}:${this.port}:${this.natType}) from ${packet.message.peerId} (${address}:${port}:${packet.message.natType})`)

      const portCache = {}
      const getPort = portGenerator(DEFAULT_PORT, DEFAULT_TEST_PORT)
      const pingId = Math.random().toString(16).slice(2)

      const encoded = await Packet.encode(new PacketPing({
        clusterId: this.clusterId,
        message: {
          peerId: this.peerId,
          natType: this.natType,
          isConnection: true,
          pingId
        }
      }))

      if (localEasy && remoteHard) {
        // debug(`onintro (${this.peerId} easy, ${packet.message.peerId} hard)`)
        peer.connecting = true

        let i = 0
        this.timer(0, 10, async (_ts) => {
          // send messages until we receive a message from them. giveup after
          // sending 1000 packets. 50% of the time 250 messages should be enough.
          if (i++ > 512) {
            peer.connecting = false
            return false
          }

          // if we got a pong packet with an id that matches the id ping packet,
          // the peer will have that pingId on it so we should check for it.
          const pair = this.peers.find(p => p.pingId === pingId)

          if (pair) {
            // debug('intro - connected:', i, s)
            pair.lastUpdate = Date.now()
            peer.connecting = false
            this.onConnection(pair, packet, pair.port, pair.address)
            return false
          }

          await this.send(encoded, getPort(portCache), packet.message.address)
        })

        return
      }

      if (localHard && remoteEasy) {
        // debug(`onintro (${this.peerId} hard, ${packet.message.peerId} easy)`)
        if (!this.bdpCache.length) {
          this.bdpCache = Array.from({ length: 256 }, () => getPort(portCache))
        }

        for (const port of this.bdpCache) {
          this.send(encoded, port, packet.message.address)
        }
        return
      }

      // this.onConnection(peer, packet, port, address)

      // debug(`onintro (${this.peerId} easy/static, ${packet.message.peerId} easy/static)`)
      this.ping(peer, { peerId: this.peerId, natType: this.natType, isConnection: true, pingId: packet.message.pingId })
    }

    //
    // A peer is letting us know they want to join to a cluster
    //  - update the peer in this.peers to include the cluster
    //  - tell the peer what heads we have with the specified clusterId
    //  - tell the peer what tails we have to see if they can help with missing packets
    //  - introduce this peer to some random peers in this.peers (introductions
    //    are only made to live peers, we do not track dead peers -- our TTL is
    //    30s so this would create a huge amount of useless data sent over the network.
    async onJoin (packet, port, address) {
      this.lastUpdate = Date.now()
      this.setPeer(packet, port, address)

      for (const peer of this.getPeers(packet, this.peers, 6)) {
        if (this.peerId === peer.peerId) continue
        if (this.peerId === packet.message.peerId) continue
        if (peer.peerId === packet.message.peerId) continue
        if (!peer.port || !peer.natType) continue

        // dont try to intro two hard peers, so we don't need thisHard === remoteHard check
        if (peer.natType === 'hard' && this.natType === 'hard') continue

        // tell the caller to ping a peer from the list
        const intro1 = await Packet.encode(new PacketIntro({
          clusterId: peer.clusterId,
          message: {
            peerId: peer.peerId,
            natType: peer.natType,
            address: peer.address,
            port: peer.port
          }
        }))

        // tell the peer from the list to ping the caller
        const intro2 = await Packet.encode(new PacketIntro({
          clusterId: packet.message.clusterId,
          message: {
            peerId: packet.message.peerId,
            natType: packet.message.natType,
            address: packet.message.address,
            port: packet.message.port
          }
        }))

        await new Promise(resolve => {
          this.timer(Math.random() * 2, 0, async () => {
            const sends = []
            // debug(`onJoin -> ${peer.address}:${peer.port}`)
            sends.push(this.send(intro2, peer.port, peer.address))

            // debug(`onJoin -> ${packet.message.address}:${packet.message.port}`)
            sends.push(this.send(intro1, packet.message.port, packet.message.address))

            await Promise.all(sends).then(resolve)
          })
        })
      }
    }

    //
    // Event is fired when a user calls .publish and a peer receives the
    // packet, or when a peer receives a join packet.
    //
    async onPub (packet, port, address, data) {
      if (this.clusterId === packet.clusterId) {
        packet.hops += 2
      } else {
        packet.hops += 1
      }

      const inserted = await this.cache.insert(packet.packetId, packet)
      if (!inserted) return

      let predicate = false

      if (inserted && this.predicateOpenPacket) {
        predicate = this.predicateOpenPacket(packet, port, address)
      } else if (inserted && this.encryption.has(packet.to)) {
        let p = { ...packet }

        if (packet.index > -1) p = await this.cache.compose(p, this)

        if (p) {
          try {
            p.message = this.encryption.open(p.message, p.to)
          } catch (e) { return this.onError(e) }

          predicate = true
          if (this.onPacket && p.index === -1) this.onPacket(p, port, address)
        } else {
          for (const peer of this.getPeers(packet, this.peers)) {
            this.negotiateCache(peer, packet, peer.port, peer.address)
          }
        }
      }

      if (packet.hops > this.maxHops || predicate === false) return
      this.mcast(packet, packet.packetId, packet.clusterId, true, [{ address, port }])
    }

    rateLimit (data, port, address) {
      if (this.peers.find(p => p.address === address && !p.clusterId)) return true

      // A peer can not send more than N messages in sequence. As messages
      // from other peers are received, their limits are reduced.
      if (!this.limits[address]) this.limits[address] = 0

      if (this.limits[address] >= 2048) { // obj faster than peers array
        if (this.limit && !this.limit(data, port, address, data)) return false
      }

      // 1Mb max allowance per peer (or until other peers get a chance)
      this.limits[address]++

      // find another peer to credit
      const otherAddress = Object.entries(this.limits)
        .sort((a, b) => b[1] - a[1])
        .filter(limits => limits[0] !== address)
        .map(limits => limits[0])
        .pop()

      if (this.limits[otherAddress]) {
        this.limits[otherAddress] -= 1

        if (this.limits[otherAddress] <= 0) {
          delete this.limits[otherAddress]
        }
      }

      return true
    }

    //
    // When a packet is received it is decoded, the packet contains the type
    // of the message. Based on the message type it is routed to a function.
    // like WebSockets, don't answer queries unless we know its another SRP peer.
    //
    onMessage (data, { port, address }) {
      if (!this.rateLimit(data, port, address)) return

      const packet = Packet.decode(data)
      const args = [packet, port, address, data]
      if (this.firewall) if (!this.firewall(...args)) return
      if (this.onData) this.onData(...args)

      switch (packet.type) {
        case PacketPing.type: return this.onPing(...args)
        case PacketPong.type: return this.onPong(...args)
        case PacketIntro.type: return this.onIntro(...args)
        case PacketJoin.type: return this.onJoin(...args)
        case PacketPublish.type: return this.onPub(...args)
        case PacketAsk.type: return this.onAskPacket(...args)
        case PacketAnswer.type: return this.onAnswerPacket(...args)
        case PacketQuery.type: return this.onQueryPacket(...args)
      }
    }
  }

  return Peer
}

export default createPeer
