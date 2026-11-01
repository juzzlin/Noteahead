"use strict";

// Audio cards: one shared <audio>, so only one clip plays at a time.
const player = new Audio();
let currentTrack = null;

const setProgress = (track, fraction) => {
    const bars = track.querySelectorAll(".wave i");
    const done = Math.round(fraction * bars.length);
    bars.forEach((bar, index) => bar.classList.toggle("done", index < done));
};

const stopCurrent = () => {
    if (currentTrack) {
        currentTrack.classList.remove("playing");
        setProgress(currentTrack, 0);
    }
    player.pause();
    currentTrack = null;
};

const playTrack = (track, fraction = 0) => {
    if (currentTrack !== track) {
        stopCurrent();
        const opus = track.dataset.src;
        const fallback = track.dataset.srcFallback;
        player.src = fallback && player.canPlayType('audio/ogg; codecs="opus"') === "" ? fallback : opus;
        currentTrack = track;
    }
    const seek = () => {
        if (fraction > 0 && player.duration) {
            player.currentTime = fraction * player.duration;
        }
    };
    player.readyState >= 1 ? seek() : player.addEventListener("loadedmetadata", seek, { once: true });
    if (fraction > 0) {
        setProgress(track, fraction);
    }
    track.classList.add("playing");
    player.play();
};

player.addEventListener("timeupdate", () => {
    if (currentTrack && player.duration) {
        setProgress(currentTrack, player.currentTime / player.duration);
    }
});

player.addEventListener("ended", stopCurrent);

document.querySelectorAll(".track").forEach((track) => {
    const wave = track.querySelector(".wave");
    const peaks = track.dataset.peaks.split(",").map(Number);
    // Mastered tracks sit near full scale, so stretch the clip's own range to show its shape.
    const low = Math.min(...peaks);
    const high = Math.max(...peaks);
    for (const peak of peaks) {
        const bar = document.createElement("i");
        bar.style.height = `${15 + 85 * (peak - low) / Math.max(high - low, 1)}%`;
        wave.appendChild(bar);
    }

    const button = track.querySelector(".play");
    if (!track.dataset.src) {
        track.classList.add("unavailable");
        button.disabled = true;
        const note = document.createElement("p");
        note.className = "soon";
        note.textContent = "Clip coming soon";
        wave.after(note);
        return;
    }

    button.addEventListener("click", () => {
        if (currentTrack === track && !player.paused) {
            player.pause();
            track.classList.remove("playing");
        } else {
            playTrack(track);
        }
    });

    wave.addEventListener("click", (event) => {
        const rect = wave.getBoundingClientRect();
        playTrack(track, (event.clientX - rect.left) / rect.width);
    });
});

// Screenshot lightbox.
const lightbox = document.querySelector(".lightbox");
const lightboxImage = lightbox.querySelector("img");

document.querySelectorAll(".gallery button").forEach((button) => {
    button.addEventListener("click", () => {
        lightboxImage.src = button.dataset.full;
        lightboxImage.alt = button.textContent.trim();
        lightbox.showModal();
    });
});

lightbox.addEventListener("click", () => lightbox.close());
