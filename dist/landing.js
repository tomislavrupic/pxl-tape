// Older local helper versions still serve index.html at /. Preserve their pairing link.
if(location.hostname==='127.0.0.1'&&(document.documentElement.dataset.zoggLocal==='true'||location.hash.startsWith('#zogg=')))location.replace('./play.html'+location.hash);
