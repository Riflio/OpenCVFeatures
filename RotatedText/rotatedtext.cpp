
/**
* @brief Выводим текст с поворотом
* @details Принцип прост - от исходной картинки вырезаем кусок по размеру рамки в которой будет повёрнутый текст,
* так как текст мы можем выводить только горизонтально, то поворачиваем этот кусок фона на противоположный угол
* пишем текст,
* поворачиваем обратно и возвращаем кусок на своё место на исходной картинке
* @param img - картинка, на которую выводить
* @param text - сам текст надписи
* @param textOrg - положение текста (левый верхний угол)
* @param angle - на сколько градусов поворачивать
* @param fontFace - стиль начертания
* @param fontScale - маштаб
* @param color - цвет
*/
static void putRotatedText(cv::Mat img, const cv::String &text, const cv::Point& textOrg, double angle, int fontFace, double fontScale, const cv::Scalar& color, int thickness, int lineType)
{
  if ( text.length()==0 ) { return; }

  //-- Узнаём размер ограничительной рамки исходного текста
  const uint8_t margin =1; //-- Запас вокруг рамки текста, что бы не геммороиться с учётом погрешности округлений и т.д.
  int baseLine =0;
  cv::Size txtBB =cv::getTextSize(text, fontFace, fontScale, thickness, &baseLine);
  txtBB +=cv::Size(0, baseLine); //-- Нужно учитывать baseLine

  //-- Создаём матрицу вращения с поворотом на нужный угол относительно исходной точки
  cv::Mat rM =cv::getRotationMatrix2D(textOrg, angle, 1.0);

  //-- Узнаём размеры и положение ограничительной рамки при повороте
  std::vector<cv::Point> txtBBPoints ={cv::Point(-margin, -margin)+textOrg, cv::Point(txtBB.width+margin, -margin)+textOrg, cv::Point(txtBB.width+margin, txtBB.height+margin)+textOrg, cv::Point(-margin, txtBB.height+margin)+textOrg};
  cv::transform(txtBBPoints, txtBBPoints, rM);

  //-- Узнаём ограничительную рамку, в которую входит рамка после поворота - что бы взять по ней кусок фона, т.к. мы копировать участок можем только соосно
  cv::Rect bgBB =cv::boundingRect(txtBBPoints);

  //-- Убеждаемся, что вообще будет что показать
  if ( bgBB.br().x<margin || bgBB.br().y<margin || bgBB.tl().x>=img.cols || bgBB.tl().y>=img.rows ) { return; }

  //-- Берём с исходной картинки фон
  cv::Mat bgImg =img(bgBB);

  //-- Поворачиваем ограничительную рамку в противоположную сторону, что бы потом текст вывести горизонтально
  std::vector<cv::Point> bgBBRevPoints ={bgBB.tl(), bgBB.tl()+cv::Point(bgBB.width, 0), bgBB.br(), bgBB.tl()+cv::Point(0, bgBB.height), textOrg};
  cv::Mat rMRev =cv::getRotationMatrix2D(txtBBPoints[0], -angle, 1.0);
  cv::transform(bgBBRevPoints, bgBBRevPoints, rMRev);

  //-- Найдём ограничительную рамку после поворота предыдущей
  cv::Rect bgBBRev =cv::boundingRect(bgBBRevPoints);

  //-- Относительно левого верхнего угла поворачиваем, при этом уберём из минусов, что бы часть картинки не потерять
  cv::Point refP =cv::Point(bgBBRevPoints[0].x-bgBBRev.tl().x, bgBBRevPoints[0].y-bgBBRev.tl().y);
  rMRev.at<double>(0, 2) =refP.x;
  rMRev.at<double>(1, 2) =refP.y;
  cv::Mat ortImg;
  cv::warpAffine(bgImg, ortImg, rMRev, bgBBRev.size());

  //-- Выводим текст
  cv::putText(ortImg, text, bgBBRevPoints[4]-bgBBRev.tl()+cv::Point(0, txtBB.height-baseLine), fontFace, fontScale, color, thickness, lineType);

  //-- Поворачиваем на прежний угол. Матрица для целевого угла у нас есть, осталось выяснить куда переносить картинку - вращаем относительно точки с предыдущего шага и переносим из минусов ей же
  const double rMA =rM.at<double>(0, 0), rMB =rM.at<double>(0, 1);
  rM.at<double>(0, 2) =(1-rMA)*refP.x - rMB*refP.y - refP.x;
  rM.at<double>(1, 2) =rMB*refP.x + (1-rMA)*refP.y - refP.y;
  cv::warpAffine(ortImg, ortImg, rM, bgBB.size());

  //-- Копируем в исходну картинку, но только отрезаем на margin по сторонам из-за округлений что бы чёрной рамки не было
  ortImg(cv::Rect(margin, margin, bgBB.width-margin*2, bgBB.height-margin*2)).copyTo(img(cv::Rect(bgBB.x+margin, bgBB.y+margin, bgBB.width-margin*2, bgBB.height-margin*2)));
}