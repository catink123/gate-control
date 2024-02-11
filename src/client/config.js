async function getConfig() {
    const file = await fetch("/config.json");
    return await file.json();
}