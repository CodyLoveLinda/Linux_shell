#!/bin/bash

nowts=$(date +%s)
echo $nowts
nowtsnum=$(echo $(date +%N) % 1000 | bc)
echo $nowtsnum
messagelist=(
"世界上最深沉的不是大海，而是你的心灵。我把自已整个都抛下，却不见泛起一丝涟漪！我好想你！"
"我是今生的水，你是前世的茶，用今生的水来泡一杯前世的茶，透明的瓷杯里，沉淀的是前世的情，沸腾的是今生的爱，这味道就叫做：缘份。"
"期待又一次落空，你是我旅途中的站台;列车与铁轨相伴，而我却留不住你的缠绵;车轮终于沉寂，啤酒、诗歌和烟，你让我挑选，我一样都不喜欢。"
"你可能只是这个世界上的一个人，但对于某人来说你就是全世界。"
"爱你是一种美丽，但想你只是一种回忆。所以我不要想你，我只会好好爱你，让我们永远在一起。"
"想你在每一个有星星的夜晚！念你在每一刻欢乐的时光！盼你在每一次想你的瞬间！爱你在每一秒呼吸的间隙！"
"亲爱的，你知道吗？如果有一千个人从我身边走过，我也可以听出你的脚步声。因为有个是踏在地上，只有你踏在我的心上！！"
"我们不是最勇敢的，却是最深情的；我们不是最走运的，却是最坚韧的；我们不是最优秀的，却是最脱俗的！诗意的叶子凋零了，我们梦想的枝干依旧挺拔。"
"不是因为寂寞才想你，而是因为想你才寂寞。孤独的感觉之所以如此之重，只是因为想得太深。"
"今天，你说你黄昏就来，已错过雨季，又错过花期，不能再错过你，那将是一生的错过呵！亲爱的，你说你黄昏就来，我情愿从黎明开始等待，等待你。"
"有时我会幻想自己是有翅膀的小天使，在夜半时分偷偷飞到你窗边，在你的脸颊上偷得一吻，可是多半的时候我会失!"
"喜欢有事没事喊你的名字；喜欢被你暖暖的手牵着；喜欢懒懒的躺在你怀里。一句话：喜欢和你在一起！"
"太不完美的我承受不起你太完美的爱……"
"我陷入深深的睡眠，明月却依旧悬挂在心空;即使你没有出现在我的门前，我也不会让它下山;这个冬天不太冷，加厚的棉衣裹不住我的透明的思恋。"
"一颗流星划过天际，我错过了许愿，一朵浪花溅上岩石，我错过了祝福，一个故事只说一遍，我错过了聆听，一段人生只走一回，不知道错误是否天定？"
"爱你是没有理由的，如果说有理由，那是因为你的存在；爱你是不能商量的，如果能商量的话是，我能不爱你吗？"
"静静的夜里，一个人偷偷想你，已成为我最隐秘的快乐。好几次梦中有你，我便贪恋着不想起床，放纵自己恣意占有你的似水柔情。"
"生活可以是平淡，犹如蓝天下碧蓝的湖水，生活也可以是诗，在一路的奔腾中高歌。只要我们牵着手，每一个日子都是幸福。"
"我是离枝的叶，你是不动的根，所以你终究是我的依附；我是跳动的山泉，你是沉默的大海，所以你注定是我的归属！"
"我爱你并不是因为你是谁，而是因为在你身边的时候我是谁。"
"世上最遥远的距离，不是生与死的距离，不是天各一方，而是我就站在你的面前，你却不知道我爱你。"
"遇见你是无意，认识你是天意，想着你是情意，不见你时三心二意，见到你便一心一意！"
"我无法保证，无法向你承诺什么，但我会做到。如果有一天你有饥饿的感觉，那时你定会看到，我已含笑饿死在你的怀抱中"
"没有太阳，月亮摸不清轨道；没有七彩，世界只剩灰白；没有雨水，海洋定会干涸；没有你的存在，我将百无聊赖"
"花儿忍不住清香，鸟儿忍不住飞翔，我忍不住对你的思念。微风忍不住吹拂，火焰忍不住燃烧，我忍不住思念你"
)

facelist=(
"[微笑]"
"[爱心]"
"[玫瑰]"
)

msgnum=$(echo $(date +%N) % 25 | bc)
facenum=$(echo $(date +%N) % 3 | bc)

echo $msgnum
echo $facenum

msg=${messagelist[$msgnum]}""${facelist[$facenum]}
#echo $msg

curl 'https://wx.qq.com/cgi-bin/mmwebwx-bin/webwxsendmsg?pass_ticket=k%252F92FRFymxs4zWm34ByPf9pEbXXFXUz4Vt0D44IMckIwUaWWh7B6JDMGfoeBQeCj' \
  -H 'Connection: keep-alive' \
  -H 'sec-ch-ua: " Not A;Brand";v="99", "Chromium";v="98", "Google Chrome";v="98"' \
  -H 'Accept: application/json, text/plain, */*' \
  -H 'Content-Type: application/json;charset=UTF-8' \
  -H 'sec-ch-ua-mobile: ?0' \
  -H 'User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/98.0.4758.102 Safari/537.36' \
  -H 'sec-ch-ua-platform: "Linux"' \
  -H 'Origin: https://wx.qq.com' \
  -H 'Sec-Fetch-Site: same-origin' \
  -H 'Sec-Fetch-Mode: cors' \
  -H 'Sec-Fetch-Dest: empty' \
  -H 'Referer: https://wx.qq.com/' \
  -H 'Accept-Language: en-US,en;q=0.9' \
  -H 'Cookie: webwxuvid=e6875f0199916b62d28366f8504834bb15b69ef436fa3bf6c16ce1dbbeb19e79; wxuin=816089826; MM_WX_NOTIFY_STATE=1; MM_WX_SOUND_STATE=1; mm_lang=en_US; wxuin=816089826; last_wxuin=816089826; wxsid=e9dpT1zFcBTqqmVY; webwx_data_ticket=gSdGlYJaWW8vsPsdrDhBJjw5; webwx_auth_ticket=CIsBEK72uu0KGmBl9w1XlWUkeoiI7IiJNuoB38+jjqrjyUJn4CIYx8moza+C78LLP01hLPQucOr9EX9TiKmyPoU3YRI9uV+aMdxLAAcyDQ1u/hf71dh8hFO0C9H1IbyCDuR51SvAic7LDgA=; login_frequency=2; wxloadtime=1647533240_expired; wxpluginkey=1647525607' \
  --data-raw '{"BaseRequest":{"Uin":816089826,"Sid":"e9dpT1zFcBTqqmVY","Skey":"@crypt_e18e83ee_9bc7521f7f2b3487633746a04fb9eb38","DeviceID":"e977968720801'$nowtsnum'"},"Msg":{"Type":1,"Content":"'$msg'","FromUserName":"@076a2ade36c2168022f94aa7766c06fcfadee282a182ba9d6401477d30fcd658","ToUserName":"@24abf62cd6f2558599b76e90a214654d","LocalID":"'$nowts'1570786","ClientMsgId":"'$nowts'1570786"},"Scene":0}' \
  --compressed
