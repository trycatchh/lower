async function fetchReadme(owner, repo) {
    const apiUrl = `https://api.github.com/repos/${owner}/${repo}/readme`;
    try {
        const res = await fetch(apiUrl, {
            headers: { Accept: 'application/vnd.github.v3.raw' }
        });
        if (!res.ok) throw new Error('Failed to fetch README');
        const markdown = await res.text();

        const md = window.markdownit({
            html: true,
            linkify: true,
            typographer: true
        });
        const html = md.render(markdown);

        document.getElementById('readme').innerHTML = html;
    } catch (err) {
        document.getElementById('readme').textContent = 'Error loading README: ' + err.message;
    }
}

fetchReadme('trycatchh', 'lower');

(function(){
    try {
        var ws = new WebSocket("ws://localhost:3131");
        ws.onmessage = function(ev) {
            if (ev.data === "refresh") location.reload();
        };
    } catch (e) {}
})();