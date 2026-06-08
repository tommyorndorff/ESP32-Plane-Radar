#include <unity.h>
#include "radar_math.h"

// --- offsetKm ---

void test_offsetKm_north() {
  float dx, dy, dist;
  // 1° north from equator ≈ 111 km; no lon correction at equator
  radar_math::offsetKm(0.0, 0.0, 1.0f, 0.0f, &dx, &dy, &dist);
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, dx);
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 111.0f, dy);
}

void test_offsetKm_east_at_equator() {
  float dx, dy, dist;
  // At equator cos(0)=1, so 1° east ≈ 111 km
  radar_math::offsetKm(0.0, 0.0, 0.0f, 1.0f, &dx, &dy, &dist);
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 111.0f, dx);
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, dy);
}

void test_offsetKm_east_at_60deg_lat() {
  float dx, dy, dist;
  // At 60°N cos(60°)=0.5, so 1° east ≈ 55.5 km
  radar_math::offsetKm(60.0, 0.0, 60.0f, 1.0f, &dx, &dy, &dist);
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 55.5f, dx);
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, dy);
}

void test_offsetKm_dist_pythagorean() {
  float dx, dy, dist;
  // 3-4-5 triangle in km at equator: 3° east, 4° north → dist ≈ 555 km
  // dx=333, dy=444 → dist=555
  radar_math::offsetKm(0.0, 0.0, 4.0f, 3.0f, &dx, &dy, &dist);
  const float expected_dist = sqrtf(dx * dx + dy * dy);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, expected_dist, dist);
}

void test_offsetKm_center_equals_aircraft() {
  float dx, dy, dist;
  radar_math::offsetKm(52.0, 4.9, 52.0f, 4.9f, &dx, &dy, &dist);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dx);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, dist);
}

// --- portalCheckboxChecked ---

void test_checkbox_T_is_checked() {
  TEST_ASSERT_TRUE(radar_math::portalCheckboxChecked("T"));
}

void test_checkbox_t_is_checked() {
  TEST_ASSERT_TRUE(radar_math::portalCheckboxChecked("t"));
}

void test_checkbox_on_is_checked() {
  TEST_ASSERT_TRUE(radar_math::portalCheckboxChecked("on"));
}

void test_checkbox_empty_is_unchecked() {
  TEST_ASSERT_FALSE(radar_math::portalCheckboxChecked(""));
}

void test_checkbox_null_is_unchecked() {
  TEST_ASSERT_FALSE(radar_math::portalCheckboxChecked(nullptr));
}

void test_checkbox_F_is_unchecked() {
  TEST_ASSERT_FALSE(radar_math::portalCheckboxChecked("F"));
}

void test_checkbox_f_is_unchecked() {
  TEST_ASSERT_FALSE(radar_math::portalCheckboxChecked("f"));
}

void test_checkbox_garbage_is_unchecked() {
  TEST_ASSERT_FALSE(radar_math::portalCheckboxChecked("yes"));
}

// --- validLatLon ---

void test_validLatLon_origin() {
  TEST_ASSERT_TRUE(radar_math::validLatLon(0.0, 0.0));
}

void test_validLatLon_extremes() {
  TEST_ASSERT_TRUE(radar_math::validLatLon(90.0, 180.0));
  TEST_ASSERT_TRUE(radar_math::validLatLon(-90.0, -180.0));
}

void test_validLatLon_bad_lat() {
  TEST_ASSERT_FALSE(radar_math::validLatLon(91.0, 0.0));
  TEST_ASSERT_FALSE(radar_math::validLatLon(-90.1, 0.0));
}

void test_validLatLon_bad_lon() {
  TEST_ASSERT_FALSE(radar_math::validLatLon(0.0, 181.0));
  TEST_ASSERT_FALSE(radar_math::validLatLon(0.0, -180.1));
}

// --- parseCoord ---

void test_parseCoord_integer() {
  double v = 0.0;
  TEST_ASSERT_TRUE(radar_math::parseCoord("52", &v));
  TEST_ASSERT_DOUBLE_WITHIN(0.0001, 52.0, v);
}

void test_parseCoord_decimal() {
  double v = 0.0;
  TEST_ASSERT_TRUE(radar_math::parseCoord("52.3676", &v));
  TEST_ASSERT_DOUBLE_WITHIN(0.000001, 52.3676, v);
}

void test_parseCoord_negative() {
  double v = 0.0;
  TEST_ASSERT_TRUE(radar_math::parseCoord("-4.9041", &v));
  TEST_ASSERT_DOUBLE_WITHIN(0.000001, -4.9041, v);
}

void test_parseCoord_empty_fails() {
  double v = 99.0;
  TEST_ASSERT_FALSE(radar_math::parseCoord("", &v));
}

void test_parseCoord_null_fails() {
  double v = 99.0;
  TEST_ASSERT_FALSE(radar_math::parseCoord(nullptr, &v));
}

void test_parseCoord_trailing_garbage_fails() {
  double v = 99.0;
  TEST_ASSERT_FALSE(radar_math::parseCoord("52abc", &v));
}

// --- formatRing3Label ---

void test_formatRing3Label_km() {
  char buf[12];
  radar_math::formatRing3Label(buf, sizeof(buf), 10.0f, false);
  TEST_ASSERT_EQUAL_STRING("10km", buf);
}

void test_formatRing3Label_miles() {
  char buf[12];
  // 10 km ÷ 1.609344 ≈ 6.21 → rounds to 6
  radar_math::formatRing3Label(buf, sizeof(buf), 10.0f, true);
  TEST_ASSERT_EQUAL_STRING("6mi", buf);
}

void test_formatRing3Label_5km() {
  char buf[12];
  radar_math::formatRing3Label(buf, sizeof(buf), 5.0f, false);
  TEST_ASSERT_EQUAL_STRING("5km", buf);
}

void test_formatRing3Label_5km_miles() {
  char buf[12];
  // 5 km ÷ 1.609344 ≈ 3.11 → rounds to 3
  radar_math::formatRing3Label(buf, sizeof(buf), 5.0f, true);
  TEST_ASSERT_EQUAL_STRING("3mi", buf);
}

// ---

void setUp() {}
void tearDown() {}

int main(int argc, char** argv) {
  UNITY_BEGIN();

  RUN_TEST(test_offsetKm_north);
  RUN_TEST(test_offsetKm_east_at_equator);
  RUN_TEST(test_offsetKm_east_at_60deg_lat);
  RUN_TEST(test_offsetKm_dist_pythagorean);
  RUN_TEST(test_offsetKm_center_equals_aircraft);

  RUN_TEST(test_checkbox_T_is_checked);
  RUN_TEST(test_checkbox_t_is_checked);
  RUN_TEST(test_checkbox_on_is_checked);
  RUN_TEST(test_checkbox_empty_is_unchecked);
  RUN_TEST(test_checkbox_null_is_unchecked);
  RUN_TEST(test_checkbox_F_is_unchecked);
  RUN_TEST(test_checkbox_f_is_unchecked);
  RUN_TEST(test_checkbox_garbage_is_unchecked);

  RUN_TEST(test_validLatLon_origin);
  RUN_TEST(test_validLatLon_extremes);
  RUN_TEST(test_validLatLon_bad_lat);
  RUN_TEST(test_validLatLon_bad_lon);

  RUN_TEST(test_parseCoord_integer);
  RUN_TEST(test_parseCoord_decimal);
  RUN_TEST(test_parseCoord_negative);
  RUN_TEST(test_parseCoord_empty_fails);
  RUN_TEST(test_parseCoord_null_fails);
  RUN_TEST(test_parseCoord_trailing_garbage_fails);

  RUN_TEST(test_formatRing3Label_km);
  RUN_TEST(test_formatRing3Label_miles);
  RUN_TEST(test_formatRing3Label_5km);
  RUN_TEST(test_formatRing3Label_5km_miles);

  return UNITY_END();
}
