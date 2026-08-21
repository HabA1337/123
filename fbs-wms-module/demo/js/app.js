(function (w) {
  var K = w.KONTUR;
  K.applySavedWaves();

  var liveIdx = 0;
  var tickN = 0;
  var toastT = null;

  function $(id) { return document.getElementById(id); }

  function route() {
    var h = (location.hash || "#/client").replace(/^#/, "");
    var p = h.split("/").filter(Boolean);
    return { name: p[0] || "waves", id: p[1] || "" };
  }

  function navOn(name) {
    var map = {
      client: "nav-client",
      waves: "nav-waves",
      wave: "nav-waves",
      feed: "nav-feed",
      kiz: "nav-kiz",
      stock: "nav-stock",
      tsd: "nav-tsd"
    };
    document.querySelectorAll(".side a").forEach(function (a) { a.classList.remove("on"); });
    var el = $(map[name] || "nav-waves");
    if (el) el.classList.add("on");
  }

  function toast(msg) {
    var t = $("toast");
    t.textContent = msg;
    t.hidden = false;
    clearTimeout(toastT);
    toastT = setTimeout(function () { t.hidden = true; }, 4200);
  }

  function clock() {
    var d = new Date();
    $("clk").textContent = K.fmtDate(d) + "  " + K.fmtTime(d);
    $("foot-clk").textContent = K.fmtTime(d);
  }

  function progHTML(pct, extra) {
    var cls = pct >= 100 ? "ok" : pct < 30 ? "warn" : "";
    return '<div class="prog ' + cls + '" title="' + pct + '%"><i style="width:' + pct + '%"></i></div> <span class="pct">' + pct + "%</span>" + (extra || "");
  }

  function workerName(id) {
    var u = K.workerById(id);
    return u.name;
  }

  function printerName(id) {
    for (var i = 0; i < K.printers.length; i++) {
      if (K.printers[i].id === id) return K.printers[i].name;
    }
    return id;
  }

  function renderClient() {
    var r = K.receiving;
    var cl = K.clientById(r.client);
    var rows = K.stock.filter(function (s) { return K.sku(s.ean).client === "ntx"; }).map(function (s) {
      var sk = K.sku(s.ean);
      return "<tr>" +
        "<td class='mono'>" + K.esc(sk.art) + "</td>" +
        "<td>" + K.esc(sk.name) + ", р." + K.esc(sk.size) + "</td>" +
        "<td class='mono cell-big' style='font-size:14px'>" + K.esc(sk.cell) + "</td>" +
        "<td class='num'>" + s.on + "</td>" +
        "<td class='num'>" + s.inb + "</td>" +
        "<td class='num'>" + s.res + "</td>" +
        "<td class='num'><b>" + s.avail + "</b></td>" +
        "<td class='mono'>" + K.esc(sk.ean) + "</td>" +
        "</tr>";
    }).join("");

    var pct = Math.round((r.accepted / r.total) * 100);
    return (
      '<div class="crumbs">кабинет клиента / ООО «НордТекс»</div>' +
      '<h1 class="pg">Остатки по ячейкам</h1>' +
      '<div class="kpi">' +
        '<div class="panel"><div class="l">договор</div><div class="v" style="font-size:16px">44/26</div><div class="s">ячейки A-12-*</div></div>' +
        '<div class="panel"><div class="l">на полке</div><div class="v">' +
          K.stock.filter(function (s) { return K.sku(s.ean).client === "ntx"; }).reduce(function (a, b) { return a + b.on; }, 0) +
        '</div><div class="s">шт по закреплённым ячейкам</div></div>' +
        '<div class="panel"><div class="l">приёмка ' + K.esc(r.id) + '</div><div class="v recv"><span id="recv-n">' + r.accepted + "</span>/" + r.total +
        '</div><div class="s">начата сегодня ' + r.started + "</div></div>" +
        '<div class="panel"><div class="l">доступно к заказу</div><div class="v">' +
          K.stock.filter(function (s) { return K.sku(s.ean).client === "ntx"; }).reduce(function (a, b) { return a + b.avail; }, 0) +
        '</div><div class="s">полка минус резерв FBS</div></div>' +
      "</div>" +
      '<div class="grid2">' +
        '<div class="panel">' +
          '<div class="ph">Номенклатура клиента <span class="sub">' + K.esc(cl.name) + "</span></div>" +
          '<div class="bar"><span class="hint">Клиент видит только свои ячейки. Общей кучи склада нет.</span></div>' +
          "<table class='t'><thead><tr>" +
            "<th>Артикул</th><th>Наименование</th><th>Ячейка</th><th>На полке</th><th>В приёмке</th><th>Резерв FBS</th><th>Доступно</th><th>ШК</th>" +
          "</tr></thead><tbody>" + rows + "</tbody></table>" +
        "</div>" +
        '<div class="panel">' +
          '<div class="ph">Приёмка онлайн <span class="sub" id="recv-pct">' + pct + "%</span></div>" +
          '<div class="pb">' +
            '<div id="recv-bar">' + progHTML(pct) + "</div>" +
            '<p>Документ <b>' + K.esc(r.id) + "</b>, статус <span class='st st-work'>на линии</span></p>" +
            '<p class="hint">Последнее: <span id="recv-last">' + K.esc(r.last.tsd) + " · " + K.esc(r.last.who) +
              " · +" + r.last.qty + " шт " + K.esc(r.last.ean) + " → " + K.esc(r.last.cell) + "</span></p>" +
            '<p class="tick" id="recv-live">обновление с ТСД приёмки идёт само</p>' +
          "</div>" +
        "</div>" +
      "</div>"
    );
  }

  function renderWaves() {
    var rows = K.waves.map(function (w0) {
      var pct = K.pct(w0);
      return "<tr data-go='#/wave/" + encodeURIComponent(w0.id) + "'>" +
        "<td><a href='#/wave/" + encodeURIComponent(w0.id) + "'>" + K.esc(w0.id) + "</a></td>" +
        "<td>" + K.esc(w0.cityName) + " <span class='hint'>" + K.esc(w0.city) + "</span></td>" +
        "<td class='mono'>" + K.esc(w0.supply) + "</td>" +
        "<td>" + w0.created + "</td>" +
        "<td class='num'>" + w0.picked + " / " + w0.total + "</td>" +
        "<td>" + progHTML(pct) + "</td>" +
        "<td>" + K.waveLabel(w0.st) + "</td>" +
        "<td>" + (w0.tsd.length ? w0.tsd.join(", ") : "—") + "</td>" +
        "</tr>";
    }).join("");

    return (
      '<div class="crumbs">склад / отгрузки FBS</div>' +
      '<h1 class="pg">Заявки на город</h1>' +
      '<div class="panel">' +
        '<div class="bar">' +
          "<span>сегодня · WB API · склад «Северная»</span>" +
          '<span class="hint">заказы с кабинета сами сели в заявку на город, не россыпью</span>' +
        "</div>" +
        "<table class='t'><thead><tr>" +
          "<th>Заявка</th><th>Город</th><th>Поставка WB</th><th>Создана</th><th>Собрано</th><th>Прогресс</th><th>Статус</th><th>ТСД</th>" +
        "</tr></thead><tbody>" + rows + "</tbody></table>" +
      "</div>"
    );
  }

  function renderWave(id) {
    var w0 = K.waveById(id);
    if (!w0) return "<p>Заявка не найдена.</p>";
    var pct = K.pct(w0);
    var canSend = w0.st !== "sent" && pct >= 100 && w0.err === 0;
    var sent = w0.st === "sent";
    var rows = (w0.shown.length ? w0.shown : []).map(function (o) {
      var trc = o.st === "err" ? "err" : o.st === "pick" ? "sel" : "";
      return "<tr class='" + trc + "'>" +
        "<td class='mono'>" + K.esc(o.id) + "</td>" +
        "<td>" + K.esc(o.name) + ", р." + K.esc(o.size) + "</td>" +
        "<td class='mono'>" + K.esc(o.art) + "</td>" +
        "<td class='mono'>" + K.esc(o.cell) + "</td>" +
        "<td>" + K.stLabel(o.st) + (o.err ? "<div class='hint'>" + K.esc(o.err) + "</div>" : "") + "</td>" +
        "<td>" + (o.kiz ? "да" : "нет") + "</td>" +
        "<td class='mono'>" + K.esc(o.sticker) + "</td>" +
        "<td>" + (o.who ? K.esc(workerName(o.who)) : "—") + "</td>" +
        "</tr>";
    }).join("");
    if (!rows) {
      rows = "<tr><td colspan='8' class='hint'>Список закрыт, заявка уже в доставке (" + K.esc(w0.sentAt || "") + ")</td></tr>";
    }

    var btn = sent
      ? '<button class="btn" disabled>уже в доставке</button> <span class="hint">передано ' + K.esc(w0.sentAt) + (w0.supplyWb ? " · " + K.esc(w0.supplyWb) : "") + "</span>"
      : '<button class="btn btn-ok" id="btn-deliver" ' + (canSend ? "" : "disabled") + '>в доставку</button> ' +
        (canSend
          ? '<span class="hint">заявка закрыта, КИЗ на месте — можно отдать в поставку WB</span>'
          : '<span class="hint">кнопка молчит, пока сборка не 100% и пока висит ошибка КИЗ</span>');

    return (
      '<div class="crumbs"><a href="#/waves">заявки FBS</a> / ' + K.esc(w0.id) + "</div>" +
      '<h1 class="pg">Заявка ' + K.esc(w0.id) + " · " + K.esc(w0.cityName) + "</h1>" +
      '<div class="kpi">' +
        '<div class="panel"><div class="l">прогресс</div><div class="v">' + pct + '%</div><div class="s">' + w0.picked + " из " + w0.total + " заказов</div></div>" +
        '<div class="panel"><div class="l">поставка</div><div class="v" style="font-size:16px">' + K.esc(w0.supply) + '</div><div class="s">город ' + K.esc(w0.city) + "</div></div>" +
        '<div class="panel"><div class="l">ошибки КИЗ</div><div class="v">' + w0.err + '</div><div class="s">пока есть ошибка — в доставку нельзя</div></div>' +
        '<div class="panel"><div class="l">ТСД на заявке</div><div class="v" style="font-size:16px">' + (w0.tsd.join(", ") || "—") + '</div><div class="s">принтер ' + K.esc(printerName(w0.printer)) + "</div></div>" +
      "</div>" +
      '<div class="panel">' +
        '<div class="bar">' + btn + "</div>" +
        "<table class='t'><thead><tr>" +
          "<th>Заказ WB</th><th>Товар</th><th>Артикул</th><th>Ячейка</th><th>Сборка</th><th>КИЗ</th><th>Стикер</th><th>Кто</th>" +
        "</tr></thead><tbody>" + rows + "</tbody></table>" +
        (w0.shown.length && w0.shown.length < w0.total
          ? '<div class="pb hint">на экране ' + w0.shown.length + " из " + w0.total + " — остальные в том же виде, не листаем на созвоне</div>"
          : "") +
      "</div>"
    );
  }

  function feedItems() {
    var s = K.store.get();
    var extra = s.extraFeed || [];
    var base = K.seedFeed.map(function (e) {
      return {
        at: Date.now() + e.t * 1000,
        who: e.who,
        kind: e.kind,
        text: e.text
      };
    });
    return extra.concat(base).sort(function (a, b) { return b.at - a.at; }).slice(0, 40);
  }

  function renderFeedList() {
    return feedItems().map(function (e) {
      var u = K.workerById(e.who);
      var cls = e.kind === "bad" ? "bad" : e.kind === "print" ? "print" : "";
      return "<li class='" + cls + "'><div class='tm'>" + K.fmtTime(e.at) + "</div><div><span class='who'>" +
        K.esc(u.name) + " · " + K.esc(u.tsd) + "</span><br>" + K.esc(e.text) + "</div></li>";
    }).join("");
  }

  function renderFeed() {
    var who = K.workers.map(function (u) {
      return "<li><span><i class='dot " + (u.on ? "" : "off") + "'></i>" + K.esc(u.name) +
        " · " + K.esc(u.tsd) + "</span><span class='hint'>" + K.esc(u.line) + "</span></li>";
    }).join("");
    return (
      '<div class="crumbs">склад / мониторинг</div>' +
      '<h1 class="pg">Лента ТСД</h1>' +
      '<div class="grid2-wide">' +
        '<div class="panel">' +
          '<div class="ph">Что происходит на линии <span class="sub">живые события, без графика</span></div>' +
          '<ul class="feed" id="feed-ul">' + renderFeedList() + "</ul>" +
        "</div>" +
        '<div class="panel">' +
          '<div class="ph">Кто на линии</div>' +
          '<div class="pb"><ul class="who-list">' + who + "</ul>" +
            '<p class="hint">Иванов и Петрова собирают МСК-0821-14. Нуриев на приёмке П-1082. Савельева закрыла Питер, ТСД сдан.</p>' +
          "</div>" +
        "</div>" +
      "</div>"
    );
  }

  function renderKiz(prefill) {
    prefill = prefill || K.demoCodes.ean;
    return (
      '<div class="crumbs">склад / маркировка</div>' +
      '<h1 class="pg">КИЗ → стикер WB</h1>' +
      '<div class="grid2-wide">' +
        '<div class="panel">' +
          '<div class="ph">Скан или ввод кода</div>' +
          '<div class="pb">' +
            '<div class="row-f">' +
              '<div class="grow"><label class="f">ШК товара, КИЗ или номер заказа</label>' +
                '<input class="inp mono" id="kiz-in" value="' + K.esc(prefill) + '" autocomplete="off">' +
              "</div>" +
              '<button class="btn" id="kiz-go">найти</button>' +
            "</div>" +
            '<div class="chips" style="margin-top:8px">' +
              '<button type="button" class="chip js-fill" data-v="' + K.demoCodes.ean + '">ШК ' + K.demoCodes.ean + "</button> " +
              '<button type="button" class="chip js-fill" data-v="' + K.demoCodes.kiz + '">КИЗ</button> ' +
              '<button type="button" class="chip js-fill" data-v="3541287654301">заказ 3541287654301</button>' +
            "</div>" +
            '<p class="hint">Цепочка как на складе: сначала товар, потом марка, потом какой стикер клеить.</p>' +
            '<div id="kiz-out"></div>' +
          "</div>" +
        "</div>" +
        '<div class="panel">' +
          '<div class="ph">Журнал печати</div>' +
          '<div class="pb" id="print-log">' + renderPrintLog() + "</div>" +
        "</div>" +
      "</div>"
    );
  }

  function renderPrintLog() {
    var s = K.store.get();
    var log = s.printLog || [
      { t: Date.now() - 40000, who: "ТСД-03", sticker: "364 512 847", prn: "Zebra-МСК-1" },
      { t: Date.now() - 22000, who: "ТСД-05", sticker: "364 512 848", prn: "Zebra-МСК-1" }
    ];
    if (!log.length) return "<p class='hint'>пока пусто</p>";
    return "<table class='t'><thead><tr><th>Время</th><th>Стикер</th><th>Кто</th><th>Принтер</th></tr></thead><tbody>" +
      log.map(function (x) {
        return "<tr><td>" + K.fmtTime(x.t) + "</td><td class='mono'>" + K.esc(x.sticker) + "</td><td>" +
          K.esc(x.who) + "</td><td>" + K.esc(x.prn) + "</td></tr>";
      }).join("") + "</tbody></table>";
  }

  function fillKizOut(code) {
    var box = $("kiz-out");
    if (!box) return;
    var found = K.findByCode(code);
    if (!found) {
      box.innerHTML = '<p class="st st-bad">код не из этой поставки / не из номенклатуры стенда</p>';
      return;
    }
    var sku = found.sku;
    var order = found.order;
    if (!order) {
      var w0 = K.waveById("MSK-0821-14");
      order = w0.shown.filter(function (o) { return o.ean === sku.ean; })[0] || {
        id: "3541287654301", ean: sku.ean, sticker: "364 512 847", st: "pick"
      };
    }
    var cl = K.clientById(sku.client);
    box.innerHTML =
      '<table class="t" style="margin:10px 0"><tbody>' +
        "<tr><td>Что нашли</td><td>" + (found.type === "kiz" ? "КИЗ (DataMatrix)" : found.type === "ean" ? "ШК товара" : "заказ WB") + "</td></tr>" +
        "<tr><td>Товар</td><td>" + K.esc(sku.name) + ", р." + K.esc(sku.size) + "</td></tr>" +
        "<tr><td>Артикул / ШК</td><td class='mono'>" + K.esc(sku.art) + " · " + K.esc(sku.ean) + "</td></tr>" +
        "<tr><td>Ячейка</td><td class='mono'><b>" + K.esc(sku.cell) + "</b></td></tr>" +
        "<tr><td>Клиент</td><td>" + K.esc(cl.name) + "</td></tr>" +
        "<tr><td>Заказ WB</td><td class='mono'>" + K.esc(order.id) + "</td></tr>" +
        "<tr><td>КИЗ</td><td class='mono'>" + K.esc(sku.kiz) + "</td></tr>" +
      "</tbody></table>" +
      '<div class="row-f" style="align-items:flex-start">' +
        '<div id="sticker-box">' + K.stickerHTML(order, sku) + "</div>" +
        '<div>' +
          '<p>Клеить этот стикер. Макет WB, не свой дизайн.</p>' +
          '<button class="btn btn-ok" id="btn-print" data-sticker="' + K.esc(order.sticker) + '">на принтер Zebra-МСК-1</button>' +
          '<p class="hint">мост: ТСД / вкладка → контур → очередь RAW :9100</p>' +
          '<div id="dm-hold"></div>' +
        "</div>" +
      "</div>";
    var hold = $("dm-hold");
    if (hold) {
      hold.appendChild(document.createTextNode("КИЗ "));
      hold.appendChild(K.dmCanvas(sku.kiz, 84));
    }
  }

  function renderStock() {
    var rows = K.stock.map(function (s) {
      var sk = K.sku(s.ean);
      var cl = K.clientById(sk.client);
      return "<tr data-ean='" + sk.ean + "'>" +
        "<td class='mono'>" + K.esc(sk.cell) + "</td>" +
        "<td>" + K.esc(sk.name) + "</td>" +
        "<td class='mono'>" + K.esc(sk.art) + "</td>" +
        "<td>" + K.esc(cl.name) + "</td>" +
        "<td class='num js-on'>" + s.on + "</td>" +
        "<td class='num js-res'>" + s.res + "</td>" +
        "<td class='num js-av'>" + s.avail + "</td>" +
        "</tr>";
    }).join("");
    var jour = K.salesJournal.map(function (x) {
      var sk = K.sku(x.ean);
      return "<tr><td>" + K.esc(x.t) + "</td><td class='mono'>" + K.esc(x.order) + "</td><td>" +
        K.esc(sk.name) + "</td><td class='mono'>" + K.esc(x.cell) + "</td><td class='num'>−" + x.qty +
        "</td><td>" + K.esc(x.src) + "</td></tr>";
    }).join("");
    return (
      '<div class="crumbs">склад / остатки</div>' +
      '<h1 class="pg">Остатки и списание с WB</h1>' +
      '<div class="panel">' +
        '<div class="bar">' +
          '<button class="btn btn-warn" id="btn-sale">пришла продажа с WB</button>' +
          '<span class="hint">на созвоне жми это — с ячейки A-12-04 уйдёт 1 шт футболки, резерв тоже</span>' +
        "</div>" +
        "<table class='t' id='stock-t'><thead><tr>" +
          "<th>Ячейка</th><th>Товар</th><th>Артикул</th><th>Клиент</th><th>На полке</th><th>Резерв</th><th>Доступно</th>" +
        "</tr></thead><tbody>" + rows + "</tbody></table>" +
      "</div>" +
      '<div class="panel">' +
        '<div class="ph">Журнал списаний</div>' +
        "<table class='t' id='sale-t'><thead><tr><th>Время</th><th>Заказ WB</th><th>Товар</th><th>Ячейка</th><th>Кол-во</th><th>Источник</th></tr></thead><tbody>" +
        jour + "</tbody></table>" +
      "</div>"
    );
  }

  function renderTsdFrame() {
    return (
      '<div class="crumbs">склад / терминал</div>' +
      '<h1 class="pg">ТСД сборки</h1>' +
      '<div class="phone-wrap">' +
        '<div class="phone"><iframe src="tsd.html" title="ТСД"></iframe></div>' +
        '<div class="phone-note panel">' +
          '<div class="ph">Как показывать</div>' +
          '<div class="pb">' +
            "<p>Это тот же экран, что на телефоне. На созвоне достаточно этой рамки. С телефона в сети — <span class='mono'>/tsd.html</span>.</p>" +
            "<p>Сборщику не дают меню. Ячейка крупно, один заказ, ШК, КИЗ, стикер на принтер.</p>" +
            "<p>Коды для показа уже кнопками на ТСД. Можно вбить руками:</p>" +
            "<p class='mono'>ШК " + K.demoCodes.ean + "<br>КИЗ " + K.demoCodes.kiz + "</p>" +
            "<p class='hint'>После печати событие падает в ленту диспетчера (та же вкладка, localStorage).</p>" +
            '<p><a href="tsd.html" target="_blank">открыть ТСД отдельной вкладкой</a></p>' +
          "</div>" +
        "</div>" +
      "</div>"
    );
  }

  function page() {
    var r = route();
    navOn(r.name);
    var main = $("main");
    if (r.name === "client") main.innerHTML = renderClient();
    else if (r.name === "wave") main.innerHTML = renderWave(r.id);
    else if (r.name === "feed") main.innerHTML = renderFeed();
    else if (r.name === "kiz") {
      main.innerHTML = renderKiz();
      fillKizOut(($("kiz-in") || {}).value || K.demoCodes.ean);
    } else if (r.name === "stock") main.innerHTML = renderStock();
    else if (r.name === "tsd") main.innerHTML = renderTsdFrame();
    else main.innerHTML = renderWaves();

    $("mode").textContent = r.name === "client" ? "режим клиента · ООО «НордТекс»" : "режим склада · диспетчер";
  }

  function openDeliver() {
    var r = route();
    var w0 = K.waveById(r.id);
    if (!w0 || K.pct(w0) < 100) return;
    $("modal").hidden = false;
    $("modal").innerHTML =
      '<div class="modal-bg" id="modal-bg"><div class="modal">' +
        "<h3>В доставку</h3>" +
        '<div class="mb">Отправить заявку <b>' + K.esc(w0.id) + "</b> (" + w0.total +
          " заказов, " + K.esc(w0.cityName) + ") в поставку <span class='mono'>" + K.esc(w0.supply) +
          "</span>? После этого поставка уходит в доставку WB. Откатить с экрана нельзя.</div>" +
        '<div class="ma">' +
          '<button class="btn" id="md-no">отмена</button> ' +
          '<button class="btn btn-ok" id="md-yes">отправить</button>' +
        "</div></div></div>";
  }

  function doDeliver() {
    var r = route();
    var w0 = K.waveById(r.id);
    if (!w0) return;
    var at = K.fmtHm(new Date());
    var supplyWb = w0.supply + "-DLV";
    w0.st = "sent";
    w0.sentAt = at;
    w0.supplyWb = supplyWb;
    K.store.set({ delivered: { id: w0.id, at: at, supplyWb: supplyWb } });
    K.pushEvent({
      at: Date.now(),
      who: "iv",
      kind: "print",
      text: "заявка " + w0.id + " передана в доставку · поставка " + supplyWb
    });
    $("modal").hidden = true;
    $("modal").innerHTML = "";
    page();
    toast("Заявка " + w0.id + " ушла в доставку WB");
  }

  function doPrint(sticker) {
    var s = K.store.get();
    var log = s.printLog || [];
    log.unshift({ t: Date.now(), who: "вкладка КИЗ", sticker: sticker, prn: "Zebra-МСК-1" });
    K.store.set({ printLog: log.slice(0, 20) });
    K.pushEvent({
      at: Date.now(),
      who: "iv",
      kind: "print",
      text: "стикер WB " + sticker + " отправлен на Zebra-МСК-1 · мост контура"
    });
    var q = $("prn-q");
    if (q) q.textContent = "1";
    setTimeout(function () { if (q) q.textContent = "0"; }, 1600);
    var pl = $("print-log");
    if (pl) pl.innerHTML = renderPrintLog();
    toast("Стикер " + sticker + " в очереди Zebra-МСК-1");
  }

  function doSale() {
    var row = K.stock.filter(function (s) { return s.ean === "4600605024117"; })[0];
    if (!row || row.on <= 0) {
      toast("На полке уже ноль — для показа обнови страницу");
      return;
    }
    row.on -= 1;
    if (row.res > 0) row.res -= 1;
    row.avail = Math.max(0, row.on - row.res);
    var rec = {
      t: K.fmtHm(new Date()),
      order: "3541" + String(800000000 + (Date.now() % 100000000)).slice(-8),
      ean: row.ean,
      qty: 1,
      cell: "A-12-04",
      src: "WB sale"
    };
    K.salesJournal.unshift(rec);
    var snap = {};
    K.stock.forEach(function (s) { snap[s.ean] = { on: s.on, res: s.res, avail: s.avail }; });
    K.store.set({ stock: snap, sales: K.salesJournal.slice(0, 8) });
    page();
    toast("Продажа WB: −1 шт с ячейки A-12-04, резерв снят");
    var tr = document.querySelector("#stock-t tbody tr[data-ean='4600605024117']");
    if (tr) tr.classList.add("flash");
  }

  function liveTick() {
    tickN++;
    clock();
    if (tickN % 7 === 0 && liveIdx < K.livePool.length) {
      var ev = K.livePool[liveIdx++];
      K.pushEvent({ at: Date.now(), who: ev.who, kind: ev.kind, text: ev.text });
      if (ev.kind === "in" && K.receiving.accepted < K.receiving.total) {
        K.receiving.accepted = Math.min(K.receiving.total, K.receiving.accepted + 2);
        K.store.set({ recv: K.receiving.accepted });
      }
      var r = route();
      if (r.name === "feed") {
        var ul = $("feed-ul");
        if (ul) ul.innerHTML = renderFeedList();
      }
      if (r.name === "client") {
        var n = $("recv-n");
        var bar = $("recv-bar");
        var pctEl = $("recv-pct");
        if (n) n.textContent = K.receiving.accepted;
        if (bar) {
          var pct = Math.round((K.receiving.accepted / K.receiving.total) * 100);
          bar.innerHTML = progHTML(pct);
          if (pctEl) pctEl.textContent = pct + "%";
        }
      }
    }
  }

  document.addEventListener("click", function (e) {
    var t = e.target;
    if (t.closest && t.closest("tr[data-go]") && t.tagName !== "A") {
      location.hash = t.closest("tr[data-go]").getAttribute("data-go");
    }
    if (t.id === "btn-deliver") openDeliver();
    if (t.id === "md-no" || t.id === "modal-bg") {
      $("modal").hidden = true;
      $("modal").innerHTML = "";
    }
    if (t.id === "md-yes") doDeliver();
    if (t.id === "kiz-go") fillKizOut($("kiz-in").value);
    if (t.classList.contains("js-fill")) {
      $("kiz-in").value = t.getAttribute("data-v");
      fillKizOut($("kiz-in").value);
    }
    if (t.id === "btn-print") doPrint(t.getAttribute("data-sticker"));
    if (t.id === "btn-sale") doSale();
  });

  document.addEventListener("keydown", function (e) {
    if (e.key === "Enter" && e.target && e.target.id === "kiz-in") {
      e.preventDefault();
      fillKizOut(e.target.value);
    }
  });

  w.addEventListener("hashchange", page);
  w.addEventListener("storage", function () {
    K.applySavedWaves();
    var r = route();
    if (r.name === "feed") {
      var ul = $("feed-ul");
      if (ul) ul.innerHTML = renderFeedList();
    }
  });
  w.addEventListener("kontur-store", function () {
    var r = route();
    if (r.name === "feed") {
      var ul = $("feed-ul");
      if (ul) ul.innerHTML = renderFeedList();
    }
  });

  clock();
  if (!location.hash) location.hash = "#/client";
  else page();
  setInterval(liveTick, 1000);
})(window);
