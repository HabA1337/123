#!/usr/bin/env python3
"""Статический просмотр журнала 660 и рапорта с демо-цифрами (30 суток БД)."""

from pathlib import Path

OUT = Path(__file__).resolve().parent / "preview.html"

bd = 30
rest = bd // 3 * 2
used = 0
pay_days = rest - used
ovd, ovz = 25000, 12000
money = (ovd + ovz) / 30 * pay_days
year = 2026
fio = "Петров Пётр Петрович"
rank = "сержант"
pos = "командир отделения"

html = f"""<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="utf-8"/>
<title>Журнал 660 и рапорт</title>
<style>
  body {{ font-family: "Times New Roman", Times, serif; margin: 24px; color: #111; }}
  h1 {{ font-size: 18px; text-align: center; }}
  h2 {{ font-size: 16px; }}
  .note {{ background: #f4f1e8; padding: 10px 12px; margin: 12px 0; }}
  table {{ border-collapse: collapse; width: 100%; font-size: 12px; }}
  th, td {{ border: 1px solid #000; padding: 4px 6px; text-align: center; vertical-align: middle; }}
  th {{ background: #dce8d8; }}
  .nums {{ background: #1f4e3d; color: #fff; }}
  .yellow {{ background: #fff3b0; }}
  .right {{ text-align: right; }}
  .report {{ max-width: 720px; margin: 24px auto; line-height: 1.45; font-size: 16px; }}
  .hdr {{ text-align: right; margin-bottom: 28px; }}
  .title {{ text-align: center; font-size: 22px; font-weight: bold; margin: 24px 0; }}
  .sign {{ display: flex; justify-content: space-between; margin-top: 36px; }}
  .ok {{ color: #1f4e3d; font-weight: bold; }}
</style>
</head>
<body>
<h1>Приложение № 7 к Порядку (п. 25) — приказ МО РФ от 30.10.2015 № 660</h1>
<p class="note">Официальный журнал ведётся <b>еженедельно</b>, графы 1–13.
Боевое дежурство в нём часами не ставят — его считают сутками: за каждые 3 суток БД положено 2 суток отдыха.</p>
<p><b>на</b> <span class="yellow">1</span> неделю января {year} г. &nbsp; 1 мсв 2 мср &nbsp; в/ч 00000</p>
<table>
<tr>
  <th rowspan="3">№ п/п</th>
  <th rowspan="3">Воинская должность</th>
  <th rowspan="3">Воинское звание</th>
  <th rowspan="3">Фамилия, имя, отчество</th>
  <th rowspan="3">Учет сверхурочного времени, ч</th>
  <th rowspan="3">Учет времени привлечения в выходные и праздничные дни, ч</th>
  <th rowspan="3">Суммарное время привлечения к военной службе, ч</th>
  <th colspan="2">Учет предоставления дополнительного времени отдыха</th>
  <th colspan="2">Учет присоединенных дополнительных суток отдыха к отпуску</th>
  <th rowspan="3">Учет нереализованного времени отдыха, ч, сут.</th>
  <th rowspan="3">Подпись военнослужащего</th>
</tr>
<tr>
  <th rowspan="2">Дата</th><th rowspan="2">Время, ч</th>
  <th rowspan="2">Дата</th><th rowspan="2">Время, сут.</th>
</tr>
<tr></tr>
<tr class="nums"><th>1</th><th>2</th><th>3</th><th>4</th><th>5</th><th>6</th><th>7</th><th>8</th><th>9</th><th>10</th><th>11</th><th>12</th><th>13</th></tr>
<tr>
  <td>1</td>
  <td>{pos}</td>
  <td>{rank}</td>
  <td>{fio}</td>
  <td>0</td><td>0</td><td>0</td>
  <td class="yellow"></td><td>0</td>
  <td class="yellow"></td><td>0</td>
  <td>0</td>
  <td class="yellow"></td>
</tr>
</table>
<p>Командир (начальник) подразделения ______________ (звание, подпись, инициал, фамилия)</p>

<h2>Расчёт боевого дежурства (лист БД_сутки)</h2>
<table style="max-width:640px">
<tr><th>Показатель</th><th>Значение</th></tr>
<tr><td>Суток БД за год</td><td class="ok">{bd}</td></tr>
<tr><td>Положено суток отдыха (2 за каждые 3)</td><td class="ok">{rest}</td></tr>
<tr><td>Уже присоединено к отпуску</td><td>{used}</td></tr>
<tr><td>К выплате</td><td class="ok">{pay_days} суток</td></tr>
<tr><td>Компенсация (оклады {ovd} + {ovz}) / 30 × {pay_days}</td><td class="ok">{money:.2f} руб.</td></tr>
</table>

<div class="report">
  <div class="hdr">Командиру войсковой части 00000<br>полковнику И.И. Иванову<br><br>
  от командира отделения сержанта Петрова П.П.</div>
  <div class="title">РАПОРТ</div>
  <p>Докладываю, что в период с 01.01.{year} по 31.12.{year} я, {rank} {fio},
  привлекался к боевому дежурству (боевой службе), проводимому без ограничения
  общей продолжительности еженедельного служебного времени.</p>
  <p>Количество суток привлечения к боевому дежурству — {bd} суток.
  Согласно пункту 5 приложения № 2 к Положению о порядке прохождения военной службы
  за каждые трое суток привлечения положено двое суток отдыха.</p>
  <p>Расчёт: {bd} ÷ 3 × 2 = {rest} дополнительных суток отдыха.
  Из них предоставлено: {used} суток. Остаток: {pay_days} суток.</p>
  <p>В соответствии с пунктом 3 статьи 11 Федерального закона от 27.05.1998 № 76-ФЗ
  и приказом Министра обороны РФ от 14.02.2010 № 80 прошу выплатить денежную
  компенсацию за {pay_days} суток.</p>
  <p>Расчёт компенсации: ({ovd} + {ovz}) ÷ 30 × {pay_days} = {money:.2f} руб.</p>
  <div class="sign">
    <div>«    » ______________ {year} г.</div>
    <div>__________ / {rank} {fio} /</div>
  </div>
</div>
</body>
</html>
"""
OUT.write_text(html, encoding="utf-8")
print("wrote", OUT)


if __name__ == "__main__":
    pass
