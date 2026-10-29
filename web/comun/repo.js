/* ============================================================
   Repo: utilidades compartidas por las páginas del repositorio
   - archivos incrustados (#repoFiles) e imágenes (#repoImages)
   - resaltado de sintaxis sin librerías
   - Markdown mínimo
   - visor de código (pestaña «Código»)
   ============================================================ */
window.Repo = (() => {
"use strict";
const $ = s => document.querySelector(s), $$ = s => [...document.querySelectorAll(s)];
const esc = s => String(s).replace(/[&<>"]/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const nf = (v, d = 2) => Number.isFinite(v) ? v.toFixed(d).replace('.', ',') : '—';
const REPO = 'LuisPonce15/banco-mano-robotica', GH = 'https://github.com/' + REPO, RAW = 'https://raw.githubusercontent.com/' + REPO + '/main/';
let FILES = [], IMAGES = {};
try { FILES = JSON.parse($('#repoFiles').textContent) } catch (e) {}
try { IMAGES = JSON.parse($('#repoImages').textContent) } catch (e) {}
const store = {get(k, d) {try {const v = localStorage.getItem(k); return v == null ? d : JSON.parse(v)} catch (e) {return d}}, set(k, v) {try {localStorage.setItem(k, JSON.stringify(v))} catch (e) {}}};

/* ---------- resaltado ---------- */
const words = s => new Set(s.split(' '));
const KW_C = words('if else for while do switch case default break continue return goto sizeof new delete true false nullptr this const constexpr static volatile extern inline struct class enum union typedef using namespace template typename public private protected virtual operator auto register HIGH LOW INPUT OUTPUT INPUT_PULLUP PROGMEM');
const TY_C = words('void bool char int long short float double unsigned signed size_t uint8_t uint16_t uint32_t uint64_t int8_t int16_t int32_t int64_t String byte word boolean QueueHandle_t JsonDocument portMUX_TYPE');
const KW_PY = words('def class return if elif else for while in not and or is import from as with try except finally raise pass break continue lambda yield None True False global nonlocal assert del');
const KW_JS = words('const let var function return if else for while do switch case default break continue new delete typeof instanceof in of class extends super this null undefined true false try catch finally throw async await yield import export from');
function langOf(p) {const e = p.split('.').pop().toLowerCase();
  return {ino:'cpp',cpp:'cpp',h:'cpp',c:'cpp',py:'py',yml:'yml',yaml:'yml',md:'md',csv:'csv',html:'html',js:'js',json:'js',txt:'txt'}[e] || 'txt'}
const LANGNAME = {cpp:'C++ / Arduino',py:'Python',yml:'YAML',md:'Markdown',csv:'CSV',html:'HTML',js:'JavaScript',txt:'Texto'};
function rules(lang) {
  const id = (set, ty) => [/[A-Za-z_$][\w$]*/y, (m, src, i) => set.has(m) ? 'kw' : (ty && ty.has(m)) ? 'ty' : /^\s*\(/.test(src.slice(i + m.length, i + m.length + 40)) ? 'fn' : ''];
  const num = [/\b(?:0x[0-9a-fA-F]+|\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)[uUlLfF]*\b/y, 'num'];
  if (lang === 'cpp') return [
    [/\/\/[^\n]*/y,'com'], [/\/\*[\s\S]*?(?:\*\/|$)/y,'com'],
    [/R"(\w*)\([\s\S]*?\)\1"/y,'str'], [/"(?:\\.|[^"\\\n])*"?/y,'str'], [/'(?:\\.|[^'\\\n])*'?/y,'str'],
    [/#[ \t]*\w+/y, (m, src, i) => /(^|\n)[ \t]*$/.test(src.slice(Math.max(0, i - 80), i)) ? 'pp' : ''],
    [/<[\w.\/]+\.h>/y, (m, src, i) => /#[ \t]*include[ \t]*$/.test(src.slice(Math.max(0, i - 30), i)) ? 'str' : ''],
    num, id(KW_C, TY_C)];
  if (lang === 'py') return [[/#[^\n]*/y,'com'], [/[rRbBfF]{0,2}"""[\s\S]*?(?:"""|$)/y,'str'], [/[rRbBfF]{0,2}'''[\s\S]*?(?:'''|$)/y,'str'],
    [/[rRbBfF]{0,2}"(?:\\.|[^"\\\n])*"?/y,'str'], [/[rRbBfF]{0,2}'(?:\\.|[^'\\\n])*'?/y,'str'], [/@\w+/y,'pp'], num, id(KW_PY)];
  if (lang === 'js') return [[/\/\/[^\n]*/y,'com'], [/\/\*[\s\S]*?(?:\*\/|$)/y,'com'], [/`(?:\\.|[^`\\])*`?/y,'str'], [/"(?:\\.|[^"\\\n])*"?/y,'str'], [/'(?:\\.|[^'\\\n])*'?/y,'str'], num, id(KW_JS)];
  if (lang === 'yml') return [[/#[^\n]*/y, (m, src, i) => i === 0 || /\s/.test(src[i - 1]) ? 'com' : ''], [/(?<=^|\n)[ \t]*-?[ \t]*[\w.\-\/]+(?=:(?:\s|$))/y,'key'],
    [/"(?:\\.|[^"\\\n])*"?/y,'str'], [/'[^'\n]*'?/y,'str'], [/\$\{\{[^}]*\}\}/y,'pp'], num];
  if (lang === 'md') return [[/```[\s\S]*?(?:```|$)/y,'str'], [/(?<=^|\n)#{1,6} [^\n]*/y,'kw'], [/`[^`\n]*`/y,'str'], [/\*\*[^*\n]+\*\*/y,'ty'], [/!?\[[^\]\n]*\]\([^)\n]*\)/y,'fn'], [/(?<=^|\n)\s*(?:[-*]|\d+\.) /y,'num'], [/(?<=^|\n)>[^\n]*/y,'com']];
  if (lang === 'html') return [[/<!--[\s\S]*?(?:-->|$)/y,'com'], [/<\/?[A-Za-z][\w-]*/y,'kw'], [/(?<=\s)[\w-:@]+(?==)/y,'ty'], [/"[^"]*"?/y,'str'], [/'[^'\n]*'?/y,'str'], [/\/?>/y,'kw']];
  if (lang === 'csv') return [[/(?<=^)[^\n]*/y,'key'], [/"(?:""|[^"])*"/y,'str'], [/,/y,'com']];
  return [];
}
function highlight(src, lang) {
  const R = rules(lang), lines = [''];
  const emit = (cls, txt) => {txt.split('\n').forEach((p, k) => {if (k) lines.push(''); if (p) lines[lines.length - 1] += cls ? `<span class="tk-${cls}">${esc(p)}</span>` : esc(p)})};
  let i = 0, plain = '';
  const flush = () => {if (plain) {emit('', plain); plain = ''}};
  outer: while (i < src.length) {
    for (const [re, c] of R) {re.lastIndex = i; const m = re.exec(src); if (m && m[0].length) {const cls = typeof c === 'function' ? c(m[0], src, i) : c;
      if (cls) {flush(); emit(cls, m[0])} else plain += m[0]; i += m[0].length; continue outer}}
    plain += src[i++];
  }
  flush();
  return lines;
}

/* ---------- índice ---------- */
function outline(text, lang) {
  const L = text.split('\n'), out = [];
  if (lang === 'cpp' || lang === 'js') {
    let raw = null;
    L.forEach((ln, i) => {
      if (lang === 'cpp') {
        if (raw !== null) {if (ln.includes(')' + raw + '"')) raw = null; return}
        const r = ln.match(/R"(\w*)\(/); if (r && !ln.includes(')' + r[1] + '"', r.index + 3)) raw = r[1];
        if (/,\s*$/.test(ln) && !/^\s/.test(ln) && /\(/.test(ln) && !/[;=]/.test(ln)) ln = ln + ' ' + (L[i + 1] || '').trim();
      }
      let m = ln.match(/^\s*\/\/\s*-{3,}\s*(.+?)\s*-{3,}\s*$/); if (m) {out.push({n: i + 1, t: '§ ' + m[1], sec: 1}); return}
      if (lang === 'cpp') {m = ln.match(/^(?!\s*(?:if|for|while|switch|return|else|case|do|struct)\b)[A-Za-z_][\w\s\*&:<>,]*?\b([A-Za-z_]\w*)\s*\(([^;]*)$/);
        if (m && !/^\s/.test(ln) && (/\{\s*(\/\/.*)?$/.test(ln) || /^\s*\{/.test(L[i + 1] || '') || /\)\s*\{?\s*$/.test(ln) && !/;\s*$/.test(ln)) && !/^(?:const|static const)\b.*=/.test(ln)) out.push({n: i + 1, t: m[1] + '()'})}
      else {m = ln.match(/^\s*(?:async\s+)?function\s+(\w+)/); if (m) out.push({n: i + 1, t: m[1] + '()'})}
    });
  } else if (lang === 'py') {L.forEach((ln, i) => {const m = ln.match(/^(\s*)(def|class)\s+(\w+)/); if (m) out.push({n: i + 1, t: (m[2] === 'class' ? 'class ' : '') + m[3] + (m[2] === 'def' ? '()' : '')})})}
  else if (lang === 'md') {let fence = false; L.forEach((ln, i) => {if (/^```/.test(ln)) fence = !fence; const m = !fence && ln.match(/^(#{1,3})\s+(.*)/); if (m) out.push({n: i + 1, t: m[2].replace(/[`*]/g, ''), lvl: m[1].length})})}
  else if (lang === 'yml') {L.forEach((ln, i) => {const m = ln.match(/^( {0,2})([\w-]+):/); if (m) out.push({n: i + 1, t: m[2]})})}
  else if (lang === 'html') {L.forEach((ln, i) => {const m = ln.match(/<!--\s*([A-ZÁÉÍÓÚÑ][^-]{2,40}?)\s*-->/); if (m) out.push({n: i + 1, t: '§ ' + m[1]})})}
  return out;
}

/* ---------- Markdown ---------- */
function normPath(base, rel) {const parts = base.split('/'); parts.pop(); rel.split('/').forEach(x => {if (x === '..') parts.pop(); else if (x && x !== '.') parts.push(x)}); return parts.join('/')}
function mdInline(s, base) {
  const codes = []; s = esc(s).replace(/`([^`]+)`/g, (_, c) => {codes.push(c); return '\u0000' + (codes.length - 1) + '\u0000'});
  s = s.replace(/!\[([^\]]*)\]\(([^)\s]+)\)/g, (_, a, u) => {
    const src = /^https:/.test(u) ? u : IMAGES[normPath(base, u)];
    return src ? `<img src="${src}" alt="${a}" class="${/^https:/.test(u) ? 'badge-img' : 'md-img'}" onerror="this.replaceWith(document.createTextNode(this.alt))">` : esc(a);
  });
  s = s.replace(/\[([^\]]+)\]\(([^)\s]+)\)/g, (_, t, u) => {
    if (/^(https?:|mailto:)/.test(u)) return `<a href="${u}" target="_blank" rel="noopener">${t}</a>`;
    const clean = u.split('#')[0]; const p = clean ? normPath(base, clean) : '';
    if (p && FILES.some(f => f.path === p)) return `<a href="#" data-open="${esc(p)}">${t}</a>`;
    return `<a href="${GH}/blob/main/${esc(p || base)}" target="_blank" rel="noopener">${t}</a>`;
  });
  s = s.replace(/(?<![("=>\w])https?:\/\/[^\s<)]+[^\s<).,;:]/g, u => `<a href="${u}" target="_blank" rel="noopener">${u}</a>`);
  s = s.replace(/\*\*([^*]+)\*\*/g, '<b>$1</b>').replace(/(^|[^*\w])\*([^*\n]+)\*(?!\w)/g, '$1<i>$2</i>');
  return s.replace(/\u0000(\d+)\u0000/g, (_, k) => `<code>${codes[+k]}</code>`);
}
function mdRender(src, base, opt = {}) {
  const L = src.split('\n'); let h = '', i = 0, task = 0;
  const cells = r => r.trim().replace(/^\||\|$/g, '').split('|').map(c => c.trim());
  const ck = opt.taskKey ? store.get(opt.taskKey, {}) : null;
  while (i < L.length) {
    const ln = L[i];
    if (/^```/.test(ln)) {const lang = ln.slice(3).trim(); const buf = []; i++; while (i < L.length && !/^```/.test(L[i])) buf.push(L[i++]); i++;
      const lg = ['cpp','c','arduino'].includes(lang) ? 'cpp' : lang === 'python' ? 'py' : lang === 'yaml' ? 'yml' : lang;
      h += `<pre><code>${highlight(buf.join('\n'), lg).join('\n')}</code></pre>`; continue}
    let m = ln.match(/^(#{1,6})\s+(.*)/); if (m) {const n = Math.min(3, m[1].length); h += `<h${n} data-ln="${i + 1}">${mdInline(m[2], base)}</h${n}>`; i++; continue}
    if (/^\|/.test(ln) && /^\|?\s*:?-{2,}/.test(L[i + 1] || '')) {const head = cells(ln); i += 2; const rows = []; while (i < L.length && /^\|/.test(L[i])) rows.push(cells(L[i++]));
      h += `<div class="tbl"><table><thead><tr>${head.map(c => `<th>${mdInline(c, base)}</th>`).join('')}</tr></thead><tbody>${rows.map(r => `<tr>${r.map(c => `<td>${mdInline(c, base)}</td>`).join('')}</tr>`).join('')}</tbody></table></div>`; continue}
    if (/^>\s?/.test(ln)) {const buf = []; while (i < L.length && /^>\s?/.test(L[i])) buf.push(L[i++].replace(/^>\s?/, '')); h += `<blockquote>${mdInline(buf.join(' '), base)}</blockquote>`; continue}
    if (/^\s*[-*]\s+/.test(ln) || /^\s*\d+\.\s+/.test(ln)) {const ord = /^\s*\d+\./.test(ln); const items = [];
      while (i < L.length && (/^\s*[-*]\s+/.test(L[i]) || /^\s*\d+\.\s+/.test(L[i]) || (/^\s{2,}\S/.test(L[i]) && items.length))) {
        if (/^\s*([-*]|\d+\.)\s+/.test(L[i])) items.push(L[i].replace(/^\s*([-*]|\d+\.)\s+/, '')); else items[items.length - 1] += ' ' + L[i].trim(); i++}
      h += `<${ord ? 'ol' : 'ul'}>${items.map(t => {const tk = t.match(/^\[([ xX])\]\s+(.*)/);
        if (!tk) return `<li>${mdInline(t, base)}</li>`;
        if (ck) {const id = base + '#' + (task++); return `<li class="task"><label class="check"><input type="checkbox" data-task="${esc(id)}" ${ck[id] ? 'checked' : ''}> <span>${mdInline(tk[2], base)}</span></label></li>`}
        return `<li class="task">${tk[1] === ' ' ? '☐' : '☑'} ${mdInline(tk[2], base)}</li>`}).join('')}</${ord ? 'ol' : 'ul'}>`; continue}
    if (!ln.trim()) {i++; continue}
    const buf = []; while (i < L.length && L[i].trim() && !/^(#{1,6}\s|```|\||>|\s*[-*]\s|\s*\d+\.\s)/.test(L[i])) buf.push(L[i++]);
    if (!buf.length) buf.push(L[i++]);
    h += `<p>${mdInline(buf.join(' '), base)}</p>`;
  }
  return h;
}
function bindTasks(el, key) {
  el.addEventListener('change', e => {const c = e.target.closest('[data-task]'); if (!c) return; const st = store.get(key, {}); st[c.dataset.task] = c.checked; store.set(key, st)});
}
function file(path) {return FILES.find(f => f.path === path)}

/* ---------- visor ---------- */
function initViewer(cfg) {
  const CV = {path: null, text: '', hits: [], hi: -1, mdOn: false};
  const order = cfg.order || [];
  const dirOf = p => {const k = p.lastIndexOf('/'); return k < 0 ? '' : p.slice(0, k)};
  const files = FILES.filter(cfg.filter || (() => true));
  function buildTree() {
    const groups = {}; files.forEach(f => {const d = dirOf(f.path); (groups[d] = groups[d] || []).push(f)});
    const rank = d => {const k = order.findIndex(o => d === o || d.startsWith(o + '/')); return k < 0 ? 99 : k};
    const ks = Object.keys(groups).sort((a, b) => rank(a) - rank(b) || a.localeCompare(b));
    $('#ftree').innerHTML = ks.map(d => `<div><div class="dir">${d ? esc(d) + '/' : 'raíz'}</div>${groups[d].map(f => {
      const name = d ? f.path.slice(d.length + 1) : f.path; const meta = f.lazy ? 'en GitHub' : (f.lines + ' l');
      return `<button data-path="${esc(f.path)}" title="${esc(f.path)}"><b style="font-weight:400">${esc(name)}</b><span>${meta}</span></button>`}).join('')}</div>`).join('');
  }
  async function openFile(path, line) {
    const f = files.find(x => x.path === path) || FILES.find(x => x.path === path); if (!f) return;
    CV.path = path; store.set(cfg.storeKey, path);
    $$('#ftree button').forEach(b => b.setAttribute('aria-current', b.dataset.path === path));
    $('#cPath').textContent = path; $('#cGh').href = GH + '/blob/main/' + path;
    let text = f.text;
    if (f.lazy && text == null) {
      $('#cMeta').textContent = 'Cargando desde GitHub…'; $('#cBody').innerHTML = ''; $('#cOut').innerHTML = '';
      try {const r = await fetch(RAW + path, {cache: 'no-cache'}); if (!r.ok) throw new Error(r.status); text = f.text = await r.text(); f.lines = text.split('\n').length}
      catch (err) {if (CV.path !== path) return; $('#cMeta').textContent = ''; $('#cBody').hidden = false; $('#cDoc').hidden = true;
        $('#cBody').innerHTML = `<div class="cl"><span class="ln">!</span><span class="lc">Este archivo es la propia página (${f.size ? Math.round(f.size / 1024) + ' KB' : 'grande'}). No se pudo descargar desde aquí: ábrelo en GitHub con el botón de arriba.</span></div>`; return}
      if (CV.path !== path) return;
    }
    CV.text = text;
    const lang = langOf(path), isMd = lang === 'md';
    $('#cMd').hidden = !isMd;
    $('#cMeta').textContent = `${f.lines} líneas · ${nf(new Blob([text]).size / 1024, 1)} KB · ${LANGNAME[lang]}`;
    const lines = highlight(text, lang);
    $('#cBody').innerHTML = lines.map((h, i) => `<div class="cl" data-n="${i + 1}"><span class="ln">${i + 1}</span><span class="lc">${h || ' '}</span></div>`).join('');
    $('#cOut').innerHTML = outline(text, lang).map(o => `<button data-line="${o.n}" style="${o.lvl === 1 ? 'font-weight:600;' : ''}${o.sec ? 'color:var(--accent)' : ''}">${esc(o.t)}</button>`).join('');
    if (isMd) {$('#cDoc').innerHTML = mdRender(text, path); $('#cDoc').scrollTop = 0}
    showMd(isMd); $('#cBody').scrollTop = 0; doFind(); if (line) gotoLine(line);
  }
  function showMd(on) {$('#cDoc').hidden = !on; $('#cBody').hidden = on; $('#cMd').textContent = on ? 'Ver fuente' : 'Ver documento'; CV.mdOn = on}
  function gotoLine(n) {
    if (CV.mdOn) {let best = null; $$('#cDoc [data-ln]').forEach(h => {if (+h.dataset.ln <= n) best = h}); if (best) {const box = $('#cDoc'); box.scrollTop = best.offsetTop - box.offsetTop - 8} return}
    const el = $(`#cBody .cl[data-n="${n}"]`); if (!el) return;
    $$('#cBody .cl.cur').forEach(x => x.classList.remove('cur')); el.classList.add('cur');
    const box = $('#cBody'); box.scrollTop = el.offsetTop - box.offsetTop - box.clientHeight / 3;
  }
  function doFind() {
    const q = $('#cFind').value.trim().toLowerCase(); $$('#cBody .cl.hit').forEach(x => x.classList.remove('hit'));
    CV.hits = []; CV.hi = -1;
    if (q) {if (CV.mdOn) showMd(false); CV.text.split('\n').forEach((l, i) => {if (l.toLowerCase().includes(q)) CV.hits.push(i + 1)});
      CV.hits.forEach(n => {const el = $(`#cBody .cl[data-n="${n}"]`); if (el) el.classList.add('hit')})}
    $('#cHits').textContent = q ? String(CV.hits.length) : '';
  }
  function stepFind(d) {if (!CV.hits.length) return; CV.hi = (CV.hi + d + CV.hits.length) % CV.hits.length; gotoLine(CV.hits[CV.hi]); $('#cHits').textContent = `${CV.hi + 1} / ${CV.hits.length}`}
  $('#ftree').addEventListener('click', e => {const b = e.target.closest('button[data-path]'); if (b) openFile(b.dataset.path)});
  $('#cMd').onclick = () => showMd(!CV.mdOn);
  $('#cDoc').addEventListener('click', e => {const a = e.target.closest('a[data-open]'); if (a) {e.preventDefault(); openFile(a.dataset.open)}});
  $('#cOut').addEventListener('click', e => {const b = e.target.closest('button[data-line]'); if (b) gotoLine(+b.dataset.line)});
  $('#cBody').addEventListener('click', e => {const ln = e.target.closest('.ln'); if (!ln) return; const cl = ln.parentElement; const on = !cl.classList.contains('cur');
    $$('#cBody .cl.cur').forEach(x => x.classList.remove('cur')); if (on) cl.classList.add('cur')});
  $('#cFind').addEventListener('input', doFind);
  $('#cFind').addEventListener('keydown', e => {if (e.key === 'Enter') {e.preventDefault(); stepFind(e.shiftKey ? -1 : 1)}});
  $('#cNext').onclick = () => stepFind(1); $('#cPrev').onclick = () => stepFind(-1);
  $('#cCopy').onclick = () => {if (CV.text) cfg.copyText(CV.text)};
  $('#cDown').onclick = () => {if (CV.text) cfg.saveText(CV.text, CV.path.split('/').pop())};
  if (files.length) {buildTree(); const last = store.get(cfg.storeKey, null);
    openFile(files.some(f => f.path === last) ? last : (files.find(f => f.path === cfg.defaultFile) || files[0]).path)}
  else if ($('#cIntro')) $('#cIntro').textContent = 'Esta copia de la página no trae los archivos incrustados. Míralos en GitHub.';
  return {openFile};
}

return {$, $$, esc, nf, REPO, GH, FILES, IMAGES, store, highlight, langOf, outline, mdRender, bindTasks, file, initViewer};
})();
