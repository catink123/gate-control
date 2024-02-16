async function getConfig() {
    const file = await fetch("/config");
    return await file.json();
}