---
template: home.html
title: lzcomplexity
description: Measure randomness, structure across scales and similarity in symbolic sequences. LZ76 analysis for Python, Rust and the command line, from Pleros.
search:
  exclude: true
hide:
  - toc
---

<div class="lz-home">
<section class="lz-hero">
<div class="lz-hero-copy">
<p class="lz-eyebrow">PLEROS / SCIENTIFIC COMPUTING</p>
<h1>Lempel–Ziv<br><span>sequence analysis.</span></h1>
<p class="lz-hero-lead">Measure randomness, correlations across scales and similarity in symbolic data.</p>
<p class="lz-hero-description"><strong>lzcomplexity</strong> brings entropy density, effective measure complexity and information distance to Python, Rust and standalone command-line tools.</p>
<div class="lz-actions"><a class="lz-button lz-button-primary" href="guide/">Get started <span aria-hidden="true">→</span></a><a class="lz-text-link" href="#try-it">Try it in your browser <span aria-hidden="true">↓</span></a></div>
<div class="lz-install"><code>pip install lzcomplexity</code><button type="button" data-copy="pip install lzcomplexity" aria-label="Copy installation command">Copy <span aria-hidden="true">⧉</span></button></div>
<p class="lz-tiny">Open source · MIT licensed · Python + Rust</p>
</div>
<figure class="lz-hero-figure" aria-label="LZ76 factorization of a binary sequence">
<picture><source media="(max-width: 600px)" srcset="assets/sequence-workflow-mobile.svg"><img src="assets/sequence-workflow.svg" alt="Binary LZ76 factors: 1·0·001·1001·1101. Curved arrows alternate above and below each factor. The highlighted prefix 100 crosses earlier factor boundaries; 1001 is new." width="600" height="410" fetchpriority="high"></picture>
<figcaption><a href="guide/first-factorization/">Read the factorization example <span aria-hidden="true">↗</span></a></figcaption>
</figure>
</section>

<nav class="lz-section-nav" aria-label="On this page"><span>Explore lzcomplexity</span><a href="#measures">The measures</a><a href="#try-it">Playground</a><a href="#applications">Applications</a><a href="#documentation">Documentation <span aria-hidden="true">↗</span></a></nav>



<section class="lz-section lz-measures" id="measures">
<div class="lz-section-heading"><p class="lz-eyebrow">THE MEASURES</p><h2>Randomness, structure and similarity.</h2><p>Use h to study the production of new information, E to examine organization across ranges, and d to compare sequences.</p><a class="lz-text-link" href="guide/reading-the-numbers/">Learn to interpret the results <span aria-hidden="true">→</span></a></div>
<div class="lz-measure-list">
<a class="lz-measure" href="concepts/entropy-density/"><span class="lz-measure-symbol">h</span><div><span class="lz-measure-name">ENTROPY DENSITY</span><h3>How random is it?</h3><p>Captures how much new information a sequence produces: its unpredictability and freedom to vary. In language and creative processes, this can reflect innovation within a context.</p></div><span class="lz-measure-arrow" aria-hidden="true">↗</span></a>
<a class="lz-measure" href="concepts/emc/"><span class="lz-measure-symbol">E</span><div><span class="lz-measure-name">EFFECTIVE MEASURE COMPLEXITY</span><h3>How structured it is?</h3><p>Sensitive to correlations across different ranges and levels of organization. In text, think of the context carried by letters, words and longer passages.</p></div><span class="lz-measure-arrow" aria-hidden="true">↗</span></a>
<a class="lz-measure" href="concepts/nid/"><span class="lz-measure-symbol">d</span><div><span class="lz-measure-name">INFORMATION DISTANCE</span><h3>How different are they?</h3><p>Derived from Normalized Information Distance (NID), d compares the patterns shared by two sequences. Use it to compare recordings, texts or symbolic trajectories.</p></div><span class="lz-measure-arrow" aria-hidden="true">↗</span></a>
</div>
</section>

<section class="lz-lab" id="try-it" aria-labelledby="lz-lab-title">
<div class="lz-lab-heading"><div><p class="lz-eyebrow">INTERACTIVE PLAYGROUND</p><h2 id="lz-lab-title">Explore your own sequences.</h2><p>Paste a sequence, choose an example and explore what changes.</p></div><span class="lz-local-note"><span aria-hidden="true">●</span> Runs on your device</span></div>
<div class="lz-lab-tabs" role="tablist" aria-label="Choose a measure"><button type="button" id="lz-tab-parse" role="tab" aria-selected="true" aria-controls="lz-panel-parse" data-mode="parse">Factors &amp; h</button><button type="button" id="lz-tab-structure" role="tab" aria-selected="false" aria-controls="lz-panel-structure" data-mode="structure" tabindex="-1">Complexity E</button><button type="button" id="lz-tab-distance" role="tab" aria-selected="false" aria-controls="lz-panel-distance" data-mode="distance" tabindex="-1">Distance d</button></div>
<div class="lz-lab-grid">
<div class="lz-lab-inputs">
<div class="lz-lab-input-heading"><label for="lz-input" id="lz-input-label">Your sequence</label><span id="lz-byte-count">6 / 256 bytes</span></div>
<textarea id="lz-input" rows="4" maxlength="256" spellcheck="false" autocomplete="off" aria-describedby="lz-limit lz-input-error">banana</textarea>
<div id="lz-second-input" hidden><div class="lz-lab-input-heading"><label for="lz-input-b">Sequence B</label><span id="lz-byte-count-b">6 / 256 bytes</span></div><textarea id="lz-input-b" rows="4" maxlength="256" spellcheck="false" autocomplete="off" aria-describedby="lz-limit lz-input-error">banaba</textarea></div>
<p class="lz-input-error" id="lz-input-error" role="alert" hidden></p>
<div class="lz-presets" role="group" aria-label="Example sequences"><span>Examples</span><button type="button" data-example="banana">banana</button><button type="button" data-example="periodic">Periodic</button><button type="button" data-example="irregular">Irregular</button><button type="button" data-example="constant">Constant</button><button type="button" data-example="dna">DNA</button></div>
<div class="lz-lab-controls"><button class="lz-button lz-button-primary" type="button" id="lz-analyze">Analyze sequence <span aria-hidden="true">→</span></button><span id="lz-limit">Each input is limited to 256 UTF-8 bytes.</span></div>
<p class="lz-lab-input-note" id="lz-example-note">Start with banana to see the factor boundaries, or try a longer example to explore E.</p>
</div>
<div class="lz-lab-output" aria-live="polite" aria-atomic="true">
<p class="lz-lab-description" id="lz-result-state" hidden>Ready to analyze your new sequence.</p>
<div id="lz-panel-parse" role="tabpanel" aria-labelledby="lz-tab-parse" tabindex="0">
<div class="lz-result-heading"><span>Factorization</span><a href="guide/first-factorization/" aria-label="Read the factorization guide">About this result ↗</a></div>
<div class="lz-parse" id="lz-parse" aria-label="Pieces of the sequence"><span>b</span><span>a</span><span>n</span><span>ana</span></div>
<div class="lz-lab-results"><div><strong id="lz-demo-count">3</strong><span>LZ76 count</span></div><div><strong id="lz-demo-h">1.2925</strong><span>Entropy density h · base 2</span></div></div>
<p class="lz-lab-description" id="lz-lab-description">The final copied tail is excluded from the reported count. Short-sequence estimates of h can exceed one.</p>
</div>
<div id="lz-panel-structure" role="tabpanel" aria-labelledby="lz-tab-structure" tabindex="0" hidden>
<div class="lz-result-heading"><span>Effective measure complexity</span><a href="concepts/emc/">About E ↗</a></div>
<div class="lz-lab-results lz-structure-results"><div><strong id="lz-demo-e">—</strong><span>Effective measure complexity E</span></div><div><strong id="lz-demo-range">—</strong><span>Range examined · bytes</span></div></div>
<svg class="lz-curve" id="lz-e-chart" viewBox="0 0 480 180" role="img" aria-label="E profile across ranges"><text x="240" y="90" text-anchor="middle">Choose an example to see its scale profile.</text></svg>
<p class="lz-lab-description" id="lz-e-description">The scale profile shows the fitted score as the examined range increases.</p>
</div>
<div id="lz-panel-distance" role="tabpanel" aria-labelledby="lz-tab-distance" tabindex="0" hidden>
<div class="lz-result-heading"><span>Sequence comparison</span><a href="concepts/nid/">About distance ↗</a></div>
<div class="lz-distance-score"><strong id="lz-demo-distance">—</strong><span>Information distance · from NID</span></div>

<p class="lz-lab-description">A lower d indicates more shared patterns. Compare sequences with the same encoding and similar lengths.</p>
</div>
<p class="lz-lab-method" id="lz-lab-method">UTF-8 byte analysis · base-two logarithms · <a href="guide/quick-reference/">Python cheatsheet ↗</a></p>
</div>
</div>
<noscript><p>Enable JavaScript to use the playground, or <a href="guide/install/">install the Python library</a> to analyze sequences locally.</p></noscript>
</section>

<section class="lz-section lz-get-started">
<div class="lz-start-copy"><p class="lz-eyebrow">PYTHON, RUST AND THE COMMAND LINE</p><h2>Analyze a sequence.<br>Compare a collection.</h2><p>Factor a sequence, estimate its entropy density, explore effective measure complexity and compare it with another sequence. One Rust core powers all three interfaces.</p><div class="lz-interface-links"><a href="api/python/">Python library <span>↗</span></a><a href="cli/">Standalone binaries <span>↗</span></a><a href="api/rust/">Rust library <span>↗</span></a></div></div>
<div class="lz-code-window"><div class="lz-code-head"><span><i></i><i></i><i></i></span><span>analysis.py</span><button type="button" data-copy="import lzcomplexity as lz&#10;&#10;sequence = &quot;00010111&quot; * 256&#10;result = lz.lz76(sequence, log_base=2, max_block_size=16)&#10;&#10;print(result[&quot;h&quot;])&#10;print(result[&quot;emc&quot;][&quot;value&quot;])&#10;print(lz.nid(sequence, &quot;01010101&quot; * 256))" aria-label="Copy Python example">Copy ⧉</button></div><pre><code><span class="lz-code-purple">import</span> lzcomplexity <span class="lz-code-purple">as</span> lz

sequence = <span class="lz-code-yellow">"00010111"</span> * <span class="lz-code-orange">256</span>
result = lz.<span class="lz-code-green">lz76</span>(
    sequence,
    log_base=<span class="lz-code-orange">2</span>,
    max_block_size=<span class="lz-code-orange">16</span>,
)

<span class="lz-code-green">print</span>(result[<span class="lz-code-yellow">"h"</span>])
<span class="lz-code-green">print</span>(result[<span class="lz-code-yellow">"emc"</span>][<span class="lz-code-yellow">"value"</span>])
<span class="lz-code-green">print</span>(lz.<span class="lz-code-green">nid</span>(sequence, <span class="lz-code-yellow">"01010101"</span> * <span class="lz-code-orange">256</span>))</code></pre><div class="lz-code-bottom">pip install lzcomplexity <span>Python ≥ 3.9</span></div></div>
</section>

<section class="lz-applications" id="applications">
<div class="lz-section-heading"><p class="lz-eyebrow">FROM SYMBOLS TO SCIENCE</p><h2>Research applications.</h2><p>Use symbolic representations to study information in experimental signals, biological sequences and nonlinear dynamics.</p></div>
<div class="lz-recipe-grid">
<a class="lz-recipe" href="recipes/neuro/"><svg viewBox="0 0 300 100" aria-hidden="true"><path d="M0 51H24L30 42 36 62 42 21 48 82 54 40 60 56 66 48H104L110 39 116 70 122 18 128 79 134 39 140 56 146 49H185L191 44 197 64 203 25 209 75 215 40 221 56 227 51H300"/></svg><h3>Neural time series</h3><p>Choose a threshold, build a binary sequence and compare matched windows.</p><span class="lz-text-link">Read the recipe ↗</span></a>
<a class="lz-recipe" href="recipes/genomics/"><div class="lz-dna-art" aria-hidden="true">A C <i>G</i> T A C G <i>T</i><br>T <i>G</i> C A T G <i>C</i> A<br>G C A <i>T</i> G <i>C</i> A T</div><h3>Genomic sequences</h3><p>Read FASTA, handle ambiguous bases and make encoding choices explicit.</p><span class="lz-text-link">Read the recipe ↗</span></a>
<a class="lz-recipe" href="recipes/dynamical-systems/"><svg viewBox="0 0 300 100" aria-hidden="true"><path d="M12 85Q30 15 48 75T84 60 120 68 156 30 192 65 228 12 264 73 300 29"/><path d="M12 85Q45 10 78 56T144 38 210 54 276 20" opacity=".3"/></svg><h3>Dynamical systems</h3><p>Symbolize a trajectory and examine its transition from regular to chaotic behavior.</p><span class="lz-text-link">Read the recipe ↗</span></a>
</div>
<div class="lz-research-note"><span>THE RESEARCH</span><p>Explore <a href="https://doi.org/10.1371/journal.pone.0214863">organization in written language</a>, <a href="https://doi.org/10.1063/1.5062364">computation at the edge of chaos</a> and <a href="https://doi.org/10.1103/PhysRevE.107.044212">complexity in coupled oscillators</a>.</p><a href="project/references/">All references ↗</a></div>
</section>

<section class="lz-documentation-banner" id="documentation"><div><p class="lz-eyebrow">DOCUMENTATION</p><h2>Guides and<br>API reference.</h2><p>Installation, worked examples, detailed function and command references, and compact usage cheatsheets.</p></div><div class="lz-documentation-links"><a href="guide/">Read the documentation <span>→</span></a><a href="api/python/">Python function reference <span>→</span></a><a href="cli/">Standalone binaries <span>→</span></a><a href="guide/quick-reference/">Usage cheatsheets <span>→</span></a></div></section>
<footer class="lz-home-footer"><span><strong>lzcomplexity</strong> <span class="lz-muted">by Pleros</span></span><a href="project/authors/">Authors &amp; citation</a><a href="https://github.com/pleros-ai/lzcomplexity">GitHub <span aria-hidden="true">↗</span></a></footer>
</div>
