const installButton = document.getElementById("install-button");
const installHelp = document.getElementById("install-help");
let installPrompt;

if (window.matchMedia?.("(display-mode: standalone)").matches || navigator.standalone) {
    installHelp.hidden = true;
}

if ("serviceWorker" in navigator) {
    navigator.serviceWorker.register("./service-worker.js", { updateViaCache: "none" }).catch(() => {
        installHelp.textContent = "Die Offline-Funktion konnte nicht eingerichtet werden.";
    });
}

window.addEventListener("beforeinstallprompt", (event) => {
    event.preventDefault();
    installPrompt = event;
    installButton.hidden = false;
    installHelp.hidden = true;
});

installButton.addEventListener("click", async () => {
    if (!installPrompt) {
        return;
    }

    const prompt = installPrompt;
    installPrompt = undefined;
    installButton.hidden = true;
    try {
        await prompt.prompt();
        const { outcome } = await prompt.userChoice;
        installHelp.hidden = outcome === "accepted";
    } catch {
        installHelp.hidden = false;
    }
});

window.addEventListener("appinstalled", () => {
    installPrompt = undefined;
    installButton.hidden = true;
    installHelp.hidden = true;
});
