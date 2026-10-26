// Safari's default ambient session can obey iPhone Silent Mode even when Web Audio runs.
export function configurePlaybackSession(nav){
 try{if(nav?.audioSession&&nav.audioSession.type!=='playback')nav.audioSession.type='playback';}catch{/* Optional API: ordinary Web Audio still works without it. */}
}
