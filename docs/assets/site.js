/* Small educational LZ76 view. Production analysis remains in the Rust library. */
(function (scope) {
  "use strict";
  function analyze(text) {
    const bytes = new TextEncoder().encode(text);
    const n = bytes.length;
    if (n > 128) throw new Error("Please use at most 128 UTF-8 bytes in this demonstration.");
    let boundaries, count;
    if (!bytes.some(value => value !== bytes[0])) {
      boundaries = [0, 1, n]; count = 1;
    } else {
      boundaries = [0, 1];
      let position = 1;
      while (position < n) {
        let longest = 0;
        for (let previous = 0; previous < position; previous++) {
          let length = 0;
          while (position + length < n && bytes[previous + length] === bytes[position + length]) length++;
          longest = Math.max(longest, length);
        }
        position += longest + 1;
        boundaries.push(position);
      }
      count = boundaries.length - (boundaries[boundaries.length - 1] > n ? 2 : 1);
    }
    return { count, boundaries, bytes, h: n > 1 ? count * Math.log2(n) / n : 0 };
  }
  if (typeof module !== "undefined" && module.exports) module.exports = { analyze };
  if (typeof document === "undefined") return;

  function initialize() {
    const home = document.querySelector(".lz-home");
    if (!home || home.dataset.initialized) return;
    home.dataset.initialized = "true";
    const fixtures = {
      periodic: { sequence: "01".repeat(32), note: "Repeated patterns need fewer new pieces." },
      irregular: { sequence: "0001111011111000101101011011010111001101100101000001011110000111", note: "More new pieces; finite-length entropy estimates can exceed one." }
    };
    function specimen(name) {
      const fixture = fixtures[name], result = analyze(fixture.sequence);
      const grid = home.querySelector("#lz-symbol-grid");
      grid.replaceChildren();
      for (const bit of fixture.sequence) {
        const tile = document.createElement("span");
        tile.textContent = bit; tile.dataset.bit = bit; grid.append(tile);
      }
      home.querySelector("#lz-specimen-count").textContent = result.count;
      home.querySelector("#lz-specimen-h").textContent = result.h.toFixed(3);
      home.querySelector("#lz-specimen-note").textContent = fixture.note;
      home.querySelectorAll("[data-specimen]").forEach(button => button.setAttribute("aria-pressed", String(button.dataset.specimen === name)));
    }
    home.querySelectorAll("[data-specimen]").forEach(button => button.addEventListener("click", () => specimen(button.dataset.specimen)));
    specimen("periodic");
    const input = home.querySelector("#lz-input"), description = home.querySelector("#lz-lab-description");
    function update() {
      let result;
      try { result = analyze(input.value); }
      catch (error) { description.textContent = error.message; input.setAttribute("aria-invalid", "true"); return; }
      input.removeAttribute("aria-invalid");
      const parse = home.querySelector("#lz-parse");
      parse.replaceChildren();
      const decoder = new TextDecoder();
      for (let i = 0; i < result.boundaries.length - 1; i++) {
        const start = result.boundaries[i], end = Math.min(result.boundaries[i + 1], result.bytes.length);
        if (end <= start) continue;
        const tile = document.createElement("span");
        const value = result.bytes.slice(start, end);
        // Hex preserves byte boundaries when a parse splits a UTF-8 character.
        tile.textContent = value.some(byte => byte >= 128) ? Array.from(value, byte => byte.toString(16).padStart(2, "0")).join(" ") : decoder.decode(value).replace(/ /g, "␠").replace(/\n/g, "↵");
        tile.title = "Bytes " + start + "–" + end;
        parse.append(tile);
      }
      home.querySelector("#lz-demo-count").textContent = result.count;
      home.querySelector("#lz-demo-h").textContent = result.h.toFixed(4);
      if (!result.bytes.length) description.textContent = "Empty input uses the library’s legacy count of one and entropy density of zero; it is not an observed source.";
      else if (input.value === "banana") description.textContent = "For banana, four displayed pieces give a reported count of three: the final copied tail is excluded by the library’s counting convention. Finite-length entropy estimates can exceed one.";
      else description.textContent = "Base-two normalization, matching the current library’s counting convention. The final copied tail can be excluded from the count. Non-ASCII pieces are shown as UTF-8 byte values. This short-input demo computes no EMC-like score.";
    }
    home.querySelector("#lz-analyze").addEventListener("click", update);
    input.addEventListener("keydown", event => { if (event.key === "Enter") update(); });
    home.querySelectorAll("[data-sequence]").forEach(button => button.addEventListener("click", () => { input.value = button.dataset.sequence; update(); }));
    home.querySelectorAll("[data-copy]").forEach(button => button.addEventListener("click", async () => {
      const original = button.textContent;
      try { await navigator.clipboard.writeText(button.dataset.copy); button.textContent = "Copied ✓"; }
      catch (_) { button.textContent = "Select command to copy"; }
      setTimeout(() => { if (button.isConnected) button.textContent = original; }, 1800);
    }));
  }
  if (typeof document$ !== "undefined") document$.subscribe(initialize);
  else if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", initialize);
  else initialize();
})(typeof globalThis === "undefined" ? this : globalThis);
