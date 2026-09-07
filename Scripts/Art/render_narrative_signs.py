"""Typeset static fictional facility signs. No runtime text/render target system."""
from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
out=Path(__file__).resolve().parents[2]/'Saved/NarrativeSigns';out.mkdir(parents=True,exist_ok=True)
font='/System/Library/Fonts/STHeiti Medium.ttc'
signs={
 'WorkOrder':('米工 / 公共检修', '先把板修好', ['让每个人都用得起。','操作员：临时工   /   工单 037']),
 'Launch':('米工 / 演示转实用', '最好的 SUV', ['一千万以内 · 标准通用载具','C-01  /  成本都花在台上？']),
 'Standard':('华序 / 对比实验', '领先同行 50%', ['演示台高度：本台 3 米 / 对照台 2 米','“普及”不等于“标准足够高”。']),
 'Quiet':('白果 / 上层接口', 'Only apple can do.', ['检修完成后通行','本接口使用通用检修标准']),
 'Access':('户晨风 / 首席分等官（自封）', '苹果级人员通道', ['板修好了，人还没验。','临时加设 / 非接口规定']),
 'Carrier':('C-01 / 入轨检修设备', '苹果折叠屏', ['运动输入 → 定向冲锤','同一载体 / 通用介质']),
 'Exit':('工单 037 / 接续已恢复', '请转下一位', ['操作员：临时工','当前工单完成 · 不是世界的终点'])}
for name,(small,title,lines) in signs.items():
 im=Image.new('RGB',(1024,512),(22,31,37));d=ImageDraw.Draw(im)
 def text(pos,value,size,color):d.text(pos,value,font=ImageFont.truetype(font,size),fill=color)
 d.rectangle((48,53,88,59),fill=(208,169,97));text((108,36),small,26,(171,185,188))
 text((48,148),title,62,(237,235,219))
 d.line((48,278,976,278),fill=(66,83,89),width=2)
 for i,line in enumerate(lines):text((48,310+i*60),line,30,(182,195,196))
 im.save(out/(name+'.png'))
print(out)
