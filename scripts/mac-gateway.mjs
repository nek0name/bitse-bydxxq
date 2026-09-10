import http from 'node:http'
import { createReadStream } from 'node:fs'
import { stat } from 'node:fs/promises'

// Keep APK downloads streamed; the business server limits static file sizes.
const apk = process.env.CHARGING_APK_PATH
const backendPort = Number(process.env.CHARGING_BACKEND_PORT || 18080)
const port = Number(process.env.PORT || 80)

http.createServer(async (request, response) => {
  const pathname = new URL(request.url, 'http://localhost').pathname
  if (pathname === '/charging-mac-arm64.apk') {
    if (!['GET', 'HEAD'].includes(request.method)) {
      response.writeHead(405, { Allow: 'GET, HEAD' }).end()
      return
    }
    try {
      const file = await stat(apk)
      response.writeHead(200, {
        'Content-Type': 'application/vnd.android.package-archive',
        'Content-Length': file.size,
        'Content-Disposition': 'attachment; filename="charging-mac-arm64.apk"',
        'Cache-Control': 'no-store',
        'X-Content-Type-Options': 'nosniff',
      })
      if (request.method === 'HEAD') {
        response.end()
        return
      }
      const stream = createReadStream(apk)
      stream.on('error', () => response.destroy())
      response.on('close', () => stream.destroy())
      stream.pipe(response)
    } catch {
      response.writeHead(404).end('APK unavailable')
    }
    return
  }

  const upstream = http.request({
    hostname: '127.0.0.1',
    port: backendPort,
    path: request.url,
    method: request.method,
    headers: request.headers,
  }, result => {
    response.writeHead(result.statusCode, result.headers)
    result.on('error', () => response.destroy())
    result.pipe(response)
  })
  upstream.on('error', () => {
    if (!response.headersSent) response.writeHead(502)
    response.end('Service unavailable')
  })
  request.on('aborted', () => upstream.destroy())
  request.pipe(upstream)
}).listen(port, () => console.log(`Gateway listening on ${port}`))
