const cachePrefix = "fot25-rechentrainer-";
const cacheName = cachePrefix + "BUILD_REVISION";
const files = [
    "index.html",
    "style.css",
    "rechentrainer.js",
    "rechentrainer.wasm",
    "pwa.js",
    "manifest.webmanifest",
    "assets/frame.webp",
    "assets/defeat-title.webp",
    "assets/duration-clock.webp",
    "assets/menu-art.webp",
    "assets/results-art.webp",
    "assets/worn-paper.webp",
    "assets/pirata-one.woff2",
    "assets/icon-192.png",
    "assets/icon-512.png"
];
const scope = self.registration.scope;
const urls = new Set(files.map((file) => new URL(file, scope).href));
const entry = new URL("index.html", scope).href;

self.addEventListener("install", (event) => {
    event.waitUntil(caches.open(cacheName).then(
        (cache) => cache.addAll([...urls].map((url) => new Request(url, { cache: "reload" })))
    ));
});

self.addEventListener("activate", (event) => {
    event.waitUntil((async () => {
        const names = await caches.keys();
        await Promise.all(names.filter((name) => name.startsWith(cachePrefix) && name !== cacheName)
            .map((name) => caches.delete(name)));
        await self.clients.claim();
    })());
});

self.addEventListener("fetch", (event) => {
    if (event.request.method !== "GET") {
        return;
    }
    const url = new URL(event.request.url);
    url.search = "";
    const isEntry = event.request.mode === "navigate" && (url.href === scope || url.href === entry);
    if (!isEntry && !urls.has(url.href)) {
        return;
    }

    event.respondWith((async () => {
        const cache = await caches.open(cacheName);
        return await cache.match(isEntry ? entry : url.href) || fetch(event.request);
    })());
});
